/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dctarget.cpp
**
**/

#include "nxcore.h"
#include <npe.h>
#include <nxcore_websvc.h>

/**
 * Data collector thread pool
 */
extern ThreadPool *g_dataCollectorThreadPool;
ManualGauge64 *g_currentPollerTimer = nullptr;

/**
 * Data collector worker
 */
void DataCollector(const shared_ptr<DCObject>& dcObject);

/**
 * Throttle housekeeper if needed. Returns false if shutdown time has arrived and housekeeper process should be aborted.
 */
bool ThrottleHousekeeper();

/**
 * Lock IDATA writes
 */
void LockIDataWrites();

/**
 * Unlock IDATA writes
 */
void UnlockIDataWrites();

/**
 * Poller thread pool
 */
extern ThreadPool *g_pollerThreadPool;

/**
 * Default constructor
 */
DataCollectionTarget::DataCollectionTarget() : super(), m_statusPollState(_T("status")),
         m_configurationPollState(_T("configuration")), m_instancePollState(_T("instance"))
{
   m_deletedItems = new IntegerArray<UINT32>(32, 32);
   m_deletedTables = new IntegerArray<UINT32>(32, 32);
   m_scriptErrorReports = new StringMap();
   m_hPollerMutex = MutexCreate();
   m_proxyLoadFactor = 0;
}

/**
 * Constructor for creating new data collection capable objects
 */
DataCollectionTarget::DataCollectionTarget(const TCHAR *name) : super(name), m_statusPollState(_T("status")),
         m_configurationPollState(_T("configuration")), m_instancePollState(_T("instance"))
{
   m_deletedItems = new IntegerArray<UINT32>(32, 32);
   m_deletedTables = new IntegerArray<UINT32>(32, 32);
   m_scriptErrorReports = new StringMap();
   m_hPollerMutex = MutexCreate();
   m_proxyLoadFactor = 0;
}

/**
 * Destructor
 */
DataCollectionTarget::~DataCollectionTarget()
{
   delete m_deletedItems;
   delete m_deletedTables;
   delete m_scriptErrorReports;
   MutexDestroy(m_hPollerMutex);
}

/**
 * Delete object from database
 */
bool DataCollectionTarget::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE node_id=?"));

   // TSDB: to avoid heavy query on idata tables let collected data expire instead of deleting it immediately
   if (success && ((g_dbSyntax != DB_SYNTAX_TSDB) || !(g_flags & AF_SINGLE_TABLE_PERF_DATA)))
   {
      TCHAR query[256];
      _sntprintf(query, 256, (g_flags & AF_SINGLE_TABLE_PERF_DATA) ? _T("DELETE FROM idata WHERE item_id IN (SELECT item_id FROM items WHERE node_id=%u)") : _T("DROP TABLE idata_%u"), m_id);
      QueueSQLRequest(query);

      _sntprintf(query, 256, (g_flags & AF_SINGLE_TABLE_PERF_DATA) ? _T("DELETE FROM tdata WHERE item_id IN (SELECT item_id FROM dc_tables WHERE node_id=%u)") : _T("DROP TABLE tdata_%u"), m_id);
      QueueSQLRequest(query);
   }

   if (success)
      success = super::deleteFromDatabase(hdb);

   return success;
}

/**
 * Create NXCP message with object's data
 */
void DataCollectionTarget::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
}

/**
 * Create NXCP message with object's data - stage 2
 */
void DataCollectionTarget::fillMessageInternalStage2(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternalStage2(msg, userId);

   // Sent all DCIs marked for display on overview page or in tooltips
   UINT32 fieldIdOverview = VID_OVERVIEW_DCI_LIST_BASE;
   UINT32 countOverview = 0;
   UINT32 fieldIdTooltip = VID_TOOLTIP_DCI_LIST_BASE;
   UINT32 countTooltip = 0;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *dci = m_dcObjects->get(i);
      if ((dci->getType() == DCO_TYPE_ITEM) &&
          (dci->getStatus() == ITEM_STATUS_ACTIVE) &&
          (((DCItem *)dci)->getInstanceDiscoveryMethod() == IDM_NONE) &&
          dci->hasAccess(userId))
		{
         if  (dci->isShowInObjectOverview())
         {
            countOverview++;
            ((DCItem *)dci)->fillLastValueMessage(msg, fieldIdOverview);
            fieldIdOverview += 50;
         }
         if  (dci->isShowOnObjectTooltip())
         {
            countTooltip++;
            ((DCItem *)dci)->fillLastValueMessage(msg, fieldIdTooltip);
            fieldIdTooltip += 50;
         }
		}
	}
   unlockDciAccess();
   msg->setField(VID_OVERVIEW_DCI_COUNT, countOverview);
   msg->setField(VID_TOOLTIP_DCI_COUNT, countTooltip);
}

/**
 * Modify object from message
 */
UINT32 DataCollectionTarget::modifyFromMessageInternal(NXCPMessage *request)
{
   return super::modifyFromMessageInternal(request);
}

/**
 * Update cache for all DCI's
 */
void DataCollectionTarget::updateDciCache()
{
	readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		if (m_dcObjects->get(i)->getType() == DCO_TYPE_ITEM)
		{
			((DCItem *)m_dcObjects->get(i))->updateCacheSize();
		}
	}
	unlockDciAccess();
}

/**
 * Clean expired DCI data using TSDB drop_chunks() function
 */
void DataCollectionTarget::calculateDciCutoffTimes(time_t *cutoffTimeIData, time_t *cutoffTimeTData)
{
   time_t now = time(nullptr);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      DCObjectStorageClass sclass = o->getStorageClass();
      if (sclass == DCObjectStorageClass::DEFAULT)
         continue;

      time_t *cutoffTime = (o->getType() == DCO_TYPE_ITEM) ? &cutoffTimeIData[static_cast<int>(sclass) - 1] : &cutoffTimeTData[static_cast<int>(sclass) - 1];
      if ((*cutoffTime == 0) || (*cutoffTime > now - o->getEffectiveRetentionTime() * 86400))
         *cutoffTime = now - o->getEffectiveRetentionTime() * 86400;
   }
   unlockDciAccess();
}

/**
 * Clean expired DCI data
 */
void DataCollectionTarget::cleanDCIData(DB_HANDLE hdb)
{
   StringBuffer queryItems = _T("DELETE FROM idata");
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      queryItems.append(_T(" WHERE node_id="));
      queryItems.append(m_id);
      queryItems.append(_T(" AND "));
   }
   else
   {
      queryItems.append(_T('_'));
      queryItems.append(m_id);
      queryItems.append(_T(" WHERE "));
   }

   StringBuffer queryTables = _T("DELETE FROM tdata");
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      queryTables.append(_T(" WHERE node_id="));
      queryTables.append(m_id);
      queryTables.append(_T(" AND "));
   }
   else
   {
      queryTables.append(_T('_'));
      queryTables.append(m_id);
      queryTables.append(_T(" WHERE "));
   }

   int itemCount = 0;
   int tableCount = 0;
   time_t now = time(nullptr);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (o->getType() == DCO_TYPE_ITEM)
      {
         if (itemCount > 0)
            queryItems.append(_T(" OR "));
         queryItems.append(_T("(item_id="));
         queryItems.append(o->getId());
         queryItems.append(_T(" AND idata_timestamp<"));
         queryItems.append((INT64)(now - o->getEffectiveRetentionTime() * 86400));
         queryItems.append(_T(')'));
         itemCount++;
      }
      else if (o->getType() == DCO_TYPE_TABLE)
      {
         if (tableCount > 0)
            queryTables.append(_T(" OR "));
         queryTables.append(_T("(item_id="));
         queryTables.append(o->getId());
         queryTables.append(_T(" AND tdata_timestamp<"));
         queryTables.append((INT64)(now - o->getEffectiveRetentionTime() * 86400));
         queryTables.append(_T(')'));
         tableCount++;
      }
   }
   unlockDciAccess();

   lockProperties();
   for(int i = 0; i < m_deletedItems->size(); i++)
   {
      if (itemCount > 0)
         queryItems.append(_T(" OR "));
      queryItems.append(_T("item_id="));
      queryItems.append(m_deletedItems->get(i));
      itemCount++;
   }
   m_deletedItems->clear();

   for(int i = 0; i < m_deletedTables->size(); i++)
   {
      if (tableCount > 0)
         queryTables.append(_T(" OR "));
      queryTables.append(_T("item_id="));
      queryTables.append(m_deletedItems->get(i));
      tableCount++;
   }
   m_deletedTables->clear();
   unlockProperties();

   if (itemCount > 0)
   {
      LockIDataWrites();
      nxlog_debug_tag(_T("housekeeper"), 6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, (const TCHAR *)queryItems);
      DBQuery(hdb, queryItems);
      UnlockIDataWrites();
      if (!ThrottleHousekeeper())
         return;
   }

   if (tableCount > 0)
   {
      nxlog_debug_tag(_T("housekeeper"), 6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, (const TCHAR *)queryTables);
      DBQuery(hdb, queryTables);
   }
}

/**
 * Queue prediction engine training when necessary
 */
void DataCollectionTarget::queuePredictionEngineTraining()
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (o->getType() == DCO_TYPE_ITEM)
      {
         DCItem *dci = static_cast<DCItem*>(o);
         if (dci->getPredictionEngine()[0] != 0)
         {
            PredictionEngine *e = FindPredictionEngine(dci->getPredictionEngine());
            if ((e != nullptr) && e->requiresTraining())
            {
               QueuePredictionEngineTraining(e, dci);
            }
         }
      }
   }
   unlockDciAccess();
}

/**
 * Schedule cleanup of DCI data after DCI deletion
 */
void DataCollectionTarget::scheduleItemDataCleanup(UINT32 dciId)
{
   lockProperties();
   m_deletedItems->add(dciId);
   unlockProperties();
}

/**
 * Schedule cleanup of table DCI data after DCI deletion
 */
void DataCollectionTarget::scheduleTableDataCleanup(UINT32 dciId)
{
   lockProperties();
   m_deletedTables->add(dciId);
   unlockProperties();
}

/**
 * Get last value of DCI (either table or single value)
 */
uint32_t DataCollectionTarget::getDciLastValue(uint32_t dciId, NXCPMessage *msg)
{
   uint32_t rcc = RCC_INVALID_DCI_ID;

   readLockDciAccess();

   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if (object->getId() == dciId)
      {
         msg->setField(VID_DCOBJECT_TYPE, static_cast<int16_t>(object->getType()));
         if (object->getType() == DCO_TYPE_TABLE)
         {
            static_cast<DCTable*>(object)->fillLastValueMessage(msg);
         }
         else
         {
            static_cast<DCItem*>(object)->fillLastValueMessage(msg);
         }
         rcc = RCC_SUCCESS;
         break;
      }
   }

   unlockDciAccess();
   return rcc;
}

/**
 * Get last collected values of given table
 */
uint32_t DataCollectionTarget::getTableLastValue(uint32_t dciId, NXCPMessage *msg)
{
   uint32_t rcc = RCC_INVALID_DCI_ID;

   readLockDciAccess();

   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if (object->getId() == dciId)
		{
		   if (object->getType() == DCO_TYPE_TABLE)
		   {
		      static_cast<DCTable*>(object)->fillLastValueMessage(msg);
		      rcc = RCC_SUCCESS;
		   }
		   else
		   {
		      rcc = RCC_INCOMPATIBLE_OPERATION;
		   }
			break;
		}
	}

   unlockDciAccess();
   return rcc;
}

/**
 * Apply DCI from template
 * dcObject passed to this method should be a template's DCI
 */
bool DataCollectionTarget::applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject)
{
   bool bResult = true;

   writeLockDciAccess();	// write lock

   nxlog_debug(5, _T("Applying DCO \"%s\" to target \"%s\""), dcObject->getName().cstr(), m_name);

   // Check if that template item exists
	int i;
   for(i = 0; i < m_dcObjects->size(); i++)
      if ((m_dcObjects->get(i)->getTemplateId() == dwTemplateId) &&
          (m_dcObjects->get(i)->getTemplateItemId() == dcObject->getId()))
         break;   // Item with specified id already exist

   if (i == m_dcObjects->size())
   {
      // New item from template, just add it
		DCObject *newObject = dcObject->clone();
      newObject->setTemplateId(dwTemplateId, dcObject->getId());
      newObject->changeBinding(CreateUniqueId(IDG_ITEM), self(), TRUE);
      bResult = addDCObject(newObject, true);
   }
   else
   {
      // Update existing item unless it is disabled
      DCObject *curr = m_dcObjects->get(i);
      curr->updateFromTemplate(dcObject);
      if (curr->getInstanceDiscoveryMethod() != IDM_NONE)
      {
         updateInstanceDiscoveryItems(curr);
      }
   }

   unlockDciAccess();

	if (bResult)
	{
		lockProperties();
		setModified(MODIFY_DATA_COLLECTION, false);
		unlockProperties();
	}
   return bResult;
}

/**
 * Clean deleted template items from target's DCI list
 * Arguments is template id and list of valid template item ids.
 * all items related to given template and not presented in list should be deleted.
 */
void DataCollectionTarget::cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList)
{
   UINT32 i, j, dwNumDeleted, *pdwDeleteList;

   writeLockDciAccess();  // write lock

   pdwDeleteList = (UINT32 *)malloc(sizeof(UINT32) * m_dcObjects->size());
   dwNumDeleted = 0;

   for(i = 0; i < (UINT32)m_dcObjects->size(); i++)
      if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
      {
         for(j = 0; j < dwNumItems; j++)
            if (m_dcObjects->get(i)->getTemplateItemId() == pdwItemList[j])
               break;

         // Delete DCI if it's not in list
         if (j == dwNumItems)
            pdwDeleteList[dwNumDeleted++] = m_dcObjects->get(i)->getId();
      }

   for(i = 0; i < dwNumDeleted; i++)
      deleteDCObject(pdwDeleteList[i], false, 0);

   unlockDciAccess();
   free(pdwDeleteList);
}

/**
 * Unbind data collection target from template, i.e either remove DCI
 * association with template or remove these DCIs at all
 */
void DataCollectionTarget::unbindFromTemplate(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI)
{
   if ((getObjectClass() == OBJECT_NODE) && (templateObject->getObjectClass() == OBJECT_TEMPLATE))
   {
      static_cast<Template&>(*templateObject).removeAllPolicies(static_cast<Node*>(this));
   }

   uint32_t templateId = templateObject->getId();
   if (removeDCI)
   {
      writeLockDciAccess();  // write lock

		UINT32 *deleteList = MemAllocArray<UINT32>(m_dcObjects->size());
		int numDeleted = 0;

		int i;
      for(i = 0; i < m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == templateId)
         {
            deleteList[numDeleted++] = m_dcObjects->get(i)->getId();
         }

		for(i = 0; i < numDeleted; i++)
			deleteDCObject(deleteList[i], false, 0);

      unlockDciAccess();
		MemFree(deleteList);
   }
   else
   {
      readLockDciAccess();

      for(int i = 0; i < m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == templateId)
         {
            m_dcObjects->get(i)->setTemplateId(0, 0);
         }

      unlockDciAccess();
   }
}

/**
 * Get list of DCIs to be shown on performance tab
 */
UINT32 DataCollectionTarget::getPerfTabDCIList(NXCPMessage *pMsg, UINT32 userId)
{
	readLockDciAccess();

	UINT32 dwId = VID_SYSDCI_LIST_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if ((object->getPerfTabSettings() != nullptr) &&
          object->hasValue() &&
          (object->getStatus() == ITEM_STATUS_ACTIVE) &&
          object->matchClusterResource() &&
          object->hasAccess(userId))
		{
			pMsg->setField(dwId++, object->getId());
			pMsg->setField(dwId++, object->getDescription());
			pMsg->setField(dwId++, (WORD)object->getStatus());
			pMsg->setField(dwId++, object->getPerfTabSettings());
			pMsg->setField(dwId++, (WORD)object->getType());
			pMsg->setField(dwId++, object->getTemplateItemId());
			if (object->getType() == DCO_TYPE_ITEM)
			{
				pMsg->setField(dwId++, ((DCItem *)object)->getInstance());
            if ((object->getTemplateItemId() != 0) && (object->getTemplateId() == m_id))
            {
               // DCI created via instance discovery - send ID of root template item
               // to allow UI to resolve double template case
               // (template -> instance discovery item on node -> actual item on node)
               shared_ptr<DCObject> src = getDCObjectById(object->getTemplateItemId(), userId, false);
               pMsg->setField(dwId++, (src != nullptr) ? src->getTemplateItemId() : 0);
               dwId += 2;
            }
            else
            {
				   dwId += 3;
            }
			}
			else
			{
				dwId += 4;
			}
			dwCount++;
		}
	}
   pMsg->setField(VID_NUM_ITEMS, dwCount);

	unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Get threshold violation summary into NXCP message
 */
UINT32 DataCollectionTarget::getThresholdSummary(NXCPMessage *msg, UINT32 baseId, UINT32 userId)
{
	UINT32 varId = baseId;

	msg->setField(varId++, m_id);
	UINT32 countId = varId++;
	UINT32 count = 0;

	readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if (object->hasValue() && (object->getType() == DCO_TYPE_ITEM) && (object->getStatus() == ITEM_STATUS_ACTIVE) && object->hasAccess(userId))
		{
			if (static_cast<DCItem*>(object)->hasActiveThreshold())
			{
			   static_cast<DCItem*>(object)->fillLastValueMessage(msg, varId);
				varId += 50;
				count++;
			}
		}
	}
	unlockDciAccess();
	msg->setField(countId, count);
   return varId;
}

/**
 * Process new DCI value
 */
bool DataCollectionTarget::processNewDCValue(const shared_ptr<DCObject>& dco, time_t currTime, void *value)
{
   bool updateStatus;
	bool result = dco->processNewValue(currTime, value, &updateStatus);
	if (updateStatus)
	{
      calculateCompoundStatus(FALSE);
   }
   return result;
}

/**
 * Check if data collection is disabled
 */
bool DataCollectionTarget::isDataCollectionDisabled()
{
	return false;
}

/**
 * Put items which requires polling into the queue
 */
void DataCollectionTarget::queueItemsForPolling()
{
   if ((m_status == STATUS_UNMANAGED) || isDataCollectionDisabled() || m_isDeleted)
      return;  // Do not collect data for unmanaged objects or if data collection is disabled

   time_t currTime = time(nullptr);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
      if (m_dcObjects->get(i)->isReadyForPolling(currTime))
      {
         object->setBusyFlag();

         if ((object->getDataSource() == DS_NATIVE_AGENT) ||
             (object->getDataSource() == DS_WINPERF) ||
             (object->getDataSource() == DS_SSH) ||
             (object->getDataSource() == DS_SMCLP))
         {
            TCHAR key[32];
            _sntprintf(key, 32, _T("%08X/%s"),
                     m_id, (object->getDataSource() == DS_SSH) ? _T("ssh") :
                              (object->getDataSource() == DS_SMCLP) ? _T("smclp") : _T("agent"));
            ThreadPoolExecuteSerialized(g_dataCollectorThreadPool, key, DataCollector, m_dcObjects->getShared(i));
         }
         else
         {
            ThreadPoolExecute(g_dataCollectorThreadPool, DataCollector, m_dcObjects->getShared(i));
         }
			nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" added to queue"),
			         m_name, object->getId(), object->getName().cstr());
      }
   }
   unlockDciAccess();
}

/**
 * Update time intervals in data collection objects
 */
void DataCollectionTarget::updateDataCollectionTimeIntervals()
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      m_dcObjects->get(i)->updateTimeIntervals();
   }
   unlockDciAccess();
}

/**
 * Get object from parameter
 */
shared_ptr<NetObj> DataCollectionTarget::objectFromParameter(const TCHAR *param) const
{
   TCHAR *eptr, arg[256];
   AgentGetParameterArg(param, 1, arg, 256);
   UINT32 objectId = _tcstoul(arg, &eptr, 0);
   if (*eptr != 0)
   {
      // Argument is object's name
      objectId = 0;
   }

   // Find child object with requested ID or name
   shared_ptr<NetObj> object;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (((objectId == 0) && (!_tcsicmp(curr->getName(), arg))) ||
          (objectId == curr->getId()))
      {
         object = getChildList().getShared(i);
         break;
      }
   }
   unlockChildList();
   return object;
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError DataCollectionTarget::getInternalMetric(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
   DataCollectionError error = DCE_SUCCESS;

   if (!_tcsicmp(param, _T("PollTime.Configuration.Average")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_configurationPollState.getTimerAverage());
   }
   else if (!_tcsicmp(param, _T("PollTime.Configuration.Last")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_configurationPollState.getTimerLast());
   }
   else if (!_tcsicmp(param, _T("PollTime.Configuration.Max")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_configurationPollState.getTimerMax());
   }
   else if (!_tcsicmp(param, _T("PollTime.Configuration.Min")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_configurationPollState.getTimerMin());
   }
   else if (!_tcsicmp(param, _T("PollTime.Instance.Average")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_instancePollState.getTimerAverage());
   }
   else if (!_tcsicmp(param, _T("PollTime.Instance.Last")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_instancePollState.getTimerLast());
   }
   else if (!_tcsicmp(param, _T("PollTime.Instance.Max")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_instancePollState.getTimerMax());
   }
   else if (!_tcsicmp(param, _T("PollTime.Instance.Min")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_instancePollState.getTimerMin());
   }
   else if (!_tcsicmp(param, _T("PollTime.Status.Average")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_statusPollState.getTimerAverage());
   }
   else if (!_tcsicmp(param, _T("PollTime.Status.Last")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_statusPollState.getTimerLast());
   }
   else if (!_tcsicmp(param, _T("PollTime.Status.Max")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_statusPollState.getTimerMax());
   }
   else if (!_tcsicmp(param, _T("PollTime.Status.Min")))
   {
      _sntprintf(buffer, bufSize, INT64_FMT, m_statusPollState.getTimerMin());
   }
   else if (!_tcsicmp(param, _T("Status")))
   {
      _sntprintf(buffer, bufSize, _T("%d"), m_status);
   }
   else if (!_tcsicmp(param, _T("Dummy")) || MatchString(_T("Dummy(*)"), param, FALSE))
   {
      _tcscpy(buffer, _T("0"));
   }
   else if (MatchString(_T("ChildStatus(*)"), param, FALSE))
   {
      shared_ptr<NetObj> object = objectFromParameter(param);
      if (object != nullptr)
      {
         _sntprintf(buffer, bufSize, _T("%d"), object->getStatus());
      }
      else
      {
         error = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ConditionStatus(*)"), param, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      shared_ptr<NetObj> pObject;

      AgentGetParameterArg(param, 1, szArg, 256);
      uint32_t dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd == 0)
		{
			pObject = FindObjectById(dwId);
         if ((pObject != nullptr) && (pObject->getObjectClass() != OBJECT_CONDITION))
            pObject.reset();
		}
		else
      {
         // Argument is object's name
			pObject = FindObjectByName(szArg, OBJECT_CONDITION);
      }

      if (pObject != nullptr)
      {
			if (pObject->isTrustedNode(m_id))
			{
				_sntprintf(buffer, bufSize, _T("%d"), pObject->getStatus());
			}
			else
			{
	         error = DCE_NOT_SUPPORTED;
			}
      }
      else
      {
         error = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      error = DCE_NOT_SUPPORTED;
   }

   return error;
}

/**
 * Run data collection script. Returns pointer to NXSL VM after successful run and nullptr on failure.
 */
NXSL_VM *DataCollectionTarget::runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject)
{
   TCHAR name[256];
   _tcslcpy(name, param, 256);
   Trim(name);

   // Can be in form parameter(arg1, arg2, ... argN)
   TCHAR *p = _tcschr(name, _T('('));
   if (p != nullptr)
   {
      size_t l = _tcslen(name) - 1;
      if (name[l] != _T(')'))
         return nullptr;
      name[l] = 0;
      *p = 0;
   }

   NXSL_VM *vm = CreateServerScriptVM(name, self());
   if (vm != nullptr)
   {
      ObjectRefArray<NXSL_Value> args(16, 16);
      if ((p != nullptr) && !ParseValueList(vm, &p, args, true))
      {
         // argument parsing error
         nxlog_debug(6, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): Argument parsing error"), m_name, param);
         delete vm;
         return nullptr;
      }

      if (targetObject != nullptr)
      {
         vm->setGlobalVariable("$targetObject", targetObject->createNXSLObject(vm));
      }
      if (!vm->run(args))
      {
         nxlog_debug(6, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): Script execution error: %s"), m_name, param, vm->getErrorText());
         time_t now = time(nullptr);
         time_t lastReport = static_cast<time_t>(m_scriptErrorReports->getInt64(param, 0));
         if (lastReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
         {
            PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", name, vm->getErrorText(), m_id);
            m_scriptErrorReports->set(param, static_cast<UINT64>(now));
         }
         delete_and_null(vm);
      }
   }
   else
   {
      nxlog_debug(6, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): VM load error"), m_name, param);
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): %s"), m_name, param, (vm != nullptr) ? _T("success") : _T("failure"));
   return vm;
}

/**
 * Parse list of service call arguments
 */
static bool ParseCallArgumensList(TCHAR *input, StringList *args)
{
   TCHAR *p = input;

   TCHAR *s = p;
   int state = 1; // normal text
   for(; state > 0; p++)
   {
      switch(*p)
      {
         case '"':
            if (state == 1)
            {
               state = 2;
               s = p + 1;
            }
            else
            {
               state = 3;
               *p = 0;
               args->add(s);
            }
            break;
         case ',':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args->add(s);
               s = p + 1;
            }
            else if (state == 3)
            {
               state = 1;
               s = p + 1;
            }
            break;
         case 0:
            if (state == 1)
            {
               Trim(s);
               args->add(s);
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            else
            {
               state = -1; // error
            }
            break;
         case ' ':
            break;
         case ')':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args->add(s);
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            break;
         case '\\':
            if (state == 2)
            {
               memmove(p, p + 1, _tcslen(p) * sizeof(TCHAR));
               switch(*p)
               {
                  case 'r':
                     *p = '\r';
                     break;
                  case 'n':
                     *p = '\n';
                     break;
                  case 't':
                     *p = '\t';
                     break;
                  default:
                     break;
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         default:
            if (state == 3)
               state = -1;
            break;
      }
   }
   return (state != -1);
}

/**
 * Get item or list from web service
 * Parameter is expected in form service:path or service(arguments):path
 */
DataCollectionError DataCollectionTarget::queryWebService(const TCHAR *param, WebServiceRequestType queryType, TCHAR *buffer,
         size_t bufSize, StringList *list)
{
   uint32_t proxyId = getEffectiveWebServiceProxy();
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxyNode == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): cannot find proxy node [%u]"), m_name, param, proxyId);
      return DCE_COMM_ERROR;
   }

   TCHAR name[1024];
   _tcslcpy(name, param, 1024);
   Trim(name);

   TCHAR *path = _tcsrchr(name, _T(':'));
   if (path == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): missing parameter path"), m_name, param);
      return DCE_NOT_SUPPORTED;
   }
   *path = 0;
   path++;

   // Can be in form service(arg1, arg2, ... argN)
   StringList args;
   TCHAR *p = _tcschr(name, _T('('));
   if (p != nullptr)
   {
      size_t l = _tcslen(name) - 1;
      if (name[l] != _T(')'))
      {
         nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): error parsing argument list"), m_name, param);
         return DCE_NOT_SUPPORTED;
      }
      name[l] = 0;
      *p = 0;
      p++;
      if (!ParseCallArgumensList(p, &args))
      {
         nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): error parsing argument list"), m_name, param);
         return DCE_NOT_SUPPORTED;
      }
      Trim(name);
   }

   shared_ptr<WebServiceDefinition> d = FindWebServiceDefinition(name);
   if (d == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): cannot find web service definition"), m_name, param);
      return DCE_NOT_SUPPORTED;
   }

   shared_ptr<AgentConnectionEx> conn = proxyNode->acquireProxyConnection(WEB_SERVICE_PROXY);
   if (conn == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): cannot acquire proxy connection"), m_name, param);
      return DCE_COMM_ERROR;
   }

   StringBuffer url = expandText(d->getUrl(), nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, &args);

   StringMap headers;
   auto it = d->getHeaders().constIterator();
   while(it->hasNext())
   {
      auto h = it->next();
      StringBuffer value = expandText(h->second, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, &args);
      headers.set(h->first, value);
   }
   delete it;

   StringList pathList;
   pathList.add(path);
   StringMap results;
   uint32_t agentStatus = conn->queryWebService(queryType, url, d->getCacheRetentionTime(), d->getLogin(), d->getPassword(), d->getAuthType(),
         headers, pathList, d->isVerifyCertificate(), d->isVerifyHost(),
         (queryType == WebServiceRequestType::PARAMETER) ? static_cast<void*>(&results) : static_cast<void*>(list));

   DataCollectionError rc;
   if (agentStatus == ERR_SUCCESS)
   {
      if (queryType == WebServiceRequestType::PARAMETER)
      {
         const TCHAR *value = results.get(pathList.get(0));
         if (value != nullptr)
         {
            _tcslcpy(buffer, value, bufSize);
            rc = DCE_SUCCESS;
         }
         else
         {
            rc = DCE_NO_SUCH_INSTANCE;
         }
      }
      else
      {
         rc = DCE_SUCCESS;
      }
   }
   else
   {
      rc = DCE_COMM_ERROR;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get item from web service
 * Parameter is expected in form service:path or service(arguments):path
 */
DataCollectionError DataCollectionTarget::getMetricFromWebService(const TCHAR *param, TCHAR *buffer, size_t bufSize)
{
   return queryWebService(param, WebServiceRequestType::PARAMETER, buffer, bufSize, nullptr);
}

/**
 * Get list from library script
 * Parameter is expected in form service:path or service(arguments):path
 */
DataCollectionError DataCollectionTarget::getListFromWebService(const TCHAR *param, StringList **list)
{
   *list = new StringList();
   DataCollectionError rc = queryWebService(param, WebServiceRequestType::LIST, nullptr, 0, *list);
   if (rc != DCE_SUCCESS)
   {
      delete *list;
      *list = nullptr;
   }
   return rc;
}

/**
 * Get parameter value from NXSL script
 */
DataCollectionError DataCollectionTarget::getMetricFromScript(const TCHAR *param, TCHAR *buffer, size_t bufSize, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != nullptr)
   {
      NXSL_Value *value = vm->getResult();
      if (value->isNull())
      {
         // nullptr value is an error indicator
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         const TCHAR *dciValue = value->getValueAsCString();
         _tcslcpy(buffer, CHECK_NULL_EX(dciValue), bufSize);
         rc = DCE_SUCCESS;
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getScriptItem(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get list from library script
 */
DataCollectionError DataCollectionTarget::getListFromScript(const TCHAR *param, StringList **list, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != nullptr)
   {
      rc = DCE_SUCCESS;
      NXSL_Value *value = vm->getResult();
      if (value->isArray())
      {
         *list = value->getValueAsArray()->toStringList();
      }
      else if (value->isString())
      {
         *list = new StringList;
         (*list)->add(value->getValueAsCString());
      }
      else if (value->isNull())
      {
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         *list = new StringList;
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getListFromScript(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get table from NXSL script
 */
DataCollectionError DataCollectionTarget::getTableFromScript(const TCHAR *param, Table **result, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != nullptr)
   {
      NXSL_Value *value = vm->getResult();
      if (value->isObject(_T("Table")))
      {
         *result = (Table *)value->getValueAsObject()->getData();
         (*result)->incRefCount();
         rc = DCE_SUCCESS;
      }
      else
      {
         rc = DCE_COLLECTION_ERROR;
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getScriptTable(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get string map from library script
 */
DataCollectionError DataCollectionTarget::getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != nullptr)
   {
      rc = DCE_SUCCESS;
      NXSL_Value *value = vm->getResult();
      if (value->isHashMap())
      {
         *map = value->getValueAsHashMap()->toStringMap();
      }
      else if (value->isArray())
      {
         *map = new StringMap();
         NXSL_Array *a = value->getValueAsArray();
         for(int i = 0; i < a->size(); i++)
         {
            NXSL_Value *v = a->getByPosition(i);
            if (v->isString())
            {
               (*map)->set(v->getValueAsCString(), v->getValueAsCString());
            }
         }
      }
      else if (value->isString())
      {
         *map = new StringMap();
         (*map)->set(value->getValueAsCString(), value->getValueAsCString());
      }
      else if (value->isNull())
      {
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         *map = new StringMap();
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getListFromScript(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get last (current) DCI values for summary table.
 */
void DataCollectionTarget::getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId)
{
   if (tableDefinition->isTableDciSource())
      getTableDciValuesSummary(tableDefinition, tableData, userId);
   else
      getItemDciValuesSummary(tableDefinition, tableData, userId);
}

/**
 * Get last (current) DCI values for summary table using single-value DCIs
 */
void DataCollectionTarget::getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId)
{
   int offset = tableDefinition->isMultiInstance() ? 2 : 1;
   int baseRow = tableData->getNumRows();
   bool rowAdded = false;
   readLockDciAccess();
   for(int i = 0; i < tableDefinition->getNumColumns(); i++)
   {
      SummaryTableColumn *tc = tableDefinition->getColumn(i);
      for(int j = 0; j < m_dcObjects->size(); j++)
	   {
		   DCObject *object = m_dcObjects->get(j);
         if ((object->getType() == DCO_TYPE_ITEM) && object->hasValue() &&
             (object->getStatus() == ITEM_STATUS_ACTIVE) &&
             ((tc->m_flags & COLUMN_DEFINITION_REGEXP_MATCH) ?
               RegexpMatch(object->getName(), tc->m_dciName, FALSE) :
               !_tcsicmp(object->getName(), tc->m_dciName)
             ) && object->hasAccess(userId))
         {
            int row;
            if (tableDefinition->isMultiInstance())
            {
               // Find instance
               const TCHAR *instance = object->getInstance();
               for(row = baseRow; row < tableData->getNumRows(); row++)
               {
                  const TCHAR *v = tableData->getAsString(row, 1);
                  if (!_tcscmp(CHECK_NULL_EX(v), instance))
                     break;
               }
               if (row == tableData->getNumRows())
               {
                  tableData->addRow();
                  tableData->set(0, m_name);
                  tableData->set(1, instance);
                  tableData->setObjectId(m_id);
               }
            }
            else
            {
               if (!rowAdded)
               {
                  tableData->addRow();
                  tableData->set(0, m_name);
                  tableData->setObjectId(m_id);
                  rowAdded = true;
               }
               row = tableData->getNumRows() - 1;
            }
            tableData->setStatusAt(row, i + offset, ((DCItem *)object)->getThresholdSeverity());
            tableData->setCellObjectIdAt(row, i + offset, object->getId());
            tableData->getColumnDefinitions()->get(i + offset)->setDataType(((DCItem *)object)->getDataType());
            if (tableDefinition->getAggregationFunction() == F_LAST)
            {
               if (tc->m_flags & COLUMN_DEFINITION_MULTIVALUED)
               {
                  StringList *values = String(((DCItem *)object)->getLastValue()).split(tc->m_separator);
                  tableData->setAt(row, i + offset, values->get(0));
                  for(int r = 1; r < values->size(); r++)
                  {
                     if (row + r >= tableData->getNumRows())
                     {
                        tableData->addRow();
                        tableData->setObjectId(m_id);
                        tableData->setBaseRow(row);
                     }
                     tableData->setAt(row + r, i + offset, values->get(r));
                     tableData->setStatusAt(row + r, i + offset, ((DCItem *)object)->getThresholdSeverity());
                     tableData->setCellObjectIdAt(row + r, i + offset, object->getId());
                  }
               }
               else
               {
                  tableData->setAt(row, i + offset, ((DCItem *)object)->getLastValue());
               }
            }
            else
            {
               tableData->setPreallocatedAt(row, i + offset,
                  ((DCItem *)object)->getAggregateValue(
                     tableDefinition->getAggregationFunction(),
                     tableDefinition->getPeriodStart(),
                     tableDefinition->getPeriodEnd()));
            }

            if (!tableDefinition->isMultiInstance())
               break;
         }
      }
   }
   unlockDciAccess();
}

/**
 * Get last (current) DCI values for summary table using table DCIs
 */
void DataCollectionTarget::getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId)
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if ((o->getType() == DCO_TYPE_TABLE) && o->hasValue() &&
           (o->getStatus() == ITEM_STATUS_ACTIVE) &&
           !_tcsicmp(o->getName(), tableDefinition->getTableDciName()) &&
           o->hasAccess(userId))
      {
         Table *lastValue = ((DCTable*)o)->getLastValue();
         if (lastValue == nullptr)
            continue;

         for(int j = 0; j < lastValue->getNumRows(); j++)
         {
            tableData->addRow();
            tableData->setObjectId(m_id);
            tableData->set(0, m_name);
            for(int k = 0; k < lastValue->getNumColumns(); k++)
            {
               int columnIndex = tableData->getColumnIndex(lastValue->getColumnName(k));
               if (columnIndex == -1)
                  columnIndex = tableData->addColumn(lastValue->getColumnDefinition(k));
               tableData->set(columnIndex, lastValue->getAsString(j, k));
            }
         }
      }
   }
   unlockDciAccess();
}

/**
 * Must return true if object is a possible event source
 */
bool DataCollectionTarget::isEventSource() const
{
   return true;
}

/**
 * Returns most critical status of DCI used for
 * status calculation
 */
int DataCollectionTarget::getMostCriticalDCIStatus()
{
   int status = -1;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (curr->isStatusDCO() && (curr->getType() == DCO_TYPE_ITEM) &&
          curr->hasValue() && (curr->getStatus() == ITEM_STATUS_ACTIVE))
      {
         if (getObjectClass() == OBJECT_CLUSTER && !curr->isAggregateOnCluster())
            continue; // Calculated only on those that are aggregated on cluster

         ItemValue *value = static_cast<DCItem*>(curr)->getInternalLastValue();
         if (value != nullptr && (INT32)*value >= STATUS_NORMAL && (INT32)*value <= STATUS_CRITICAL)
            status = std::max(status, (INT32)*value);
         delete value;
      }
	}
   unlockDciAccess();
   return (status == -1) ? STATUS_UNKNOWN : status;
}

/**
 * Set object's management status
 */
bool DataCollectionTarget::setMgmtStatus(bool isManaged)
{
   return super::setMgmtStatus(isManaged);
}

/**
 * Calculate compound status
 */
void DataCollectionTarget::calculateCompoundStatus(BOOL bForcedRecalc)
{
   super::calculateCompoundStatus(bForcedRecalc);
}

/**
 * Enter maintenance mode
 */
void DataCollectionTarget::enterMaintenanceMode(uint32_t userId, const TCHAR *comments)
{
   TCHAR userName[MAX_USER_NAME];
   ResolveUserId(userId, userName, true);

   DbgPrintf(4, _T("Entering maintenance mode for %s [%d] (initiated by %s)"), m_name, m_id, userName);
   UINT64 eventId = PostSystemEvent2(EVENT_MAINTENANCE_MODE_ENTERED, m_id, "sds", CHECK_NULL_EX(comments), userId, userName);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      dco->updateThresholdsBeforeMaintenanceState();
   }
   unlockDciAccess();

   lockProperties();
   m_maintenanceEventId = eventId;
   m_maintenanceInitiator = userId;
   m_stateBeforeMaintenance = m_state;
   setModified(MODIFY_COMMON_PROPERTIES | MODIFY_DATA_COLLECTION);
   unlockProperties();
}

/**
 * Leave maintenance mode
 */
void DataCollectionTarget::leaveMaintenanceMode(uint32_t userId)
{
   TCHAR userName[MAX_USER_NAME];
   ResolveUserId(userId, userName, true);

   DbgPrintf(4, _T("Leaving maintenance mode for %s [%d] (initiated by %s)"), m_name, m_id, userName);
   PostSystemEvent(EVENT_MAINTENANCE_MODE_LEFT, m_id, "ds", userId, userName);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
      {
         continue;
      }

      dco->generateEventsBasedOnThrDiff();
   }
   unlockDciAccess();

   lockProperties();
   m_maintenanceEventId = 0;
   m_maintenanceInitiator = 0;
   bool forcePoll = m_state != m_stateBeforeMaintenance;
   m_state = m_stateBeforeMaintenance;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();

   if (forcePoll)
   {
      startForcedStatusPoll();
      TCHAR threadKey[32];
      _sntprintf(threadKey, 32, _T("POLL_%u"), getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, self(), &DataCollectionTarget::statusPollWorkerEntry, RegisterPoller(PollerType::STATUS, self()));
   }
}

/**
 * Update cache size for given data collection item
 */
void DataCollectionTarget::updateDCItemCacheSize(UINT32 dciId, UINT32 conditionId)
{
   readLockDciAccess();
   shared_ptr<DCObject> dci = getDCObjectById(dciId, 0, false);
   if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
   {
      static_cast<DCItem*>(dci.get())->updateCacheSize(conditionId);
   }
   unlockDciAccess();
}

/**
 * Reload DCI cache
 */
void DataCollectionTarget::reloadDCItemCache(UINT32 dciId)
{
   readLockDciAccess();
   shared_ptr<DCObject> dci = getDCObjectById(dciId, 0, false);
   if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
   {
      nxlog_debug_tag(_T("obj.dc.cache"), 6, _T("Reload DCI cache for \"%s\" [%d] on %s [%d]"),
               dci->getName().cstr(), dci->getId(), m_name, m_id);
      static_cast<DCItem*>(dci.get())->reloadCache(true);
   }
   unlockDciAccess();
}

/**
 * Returns true if object is data collection target
 */
bool DataCollectionTarget::isDataCollectionTarget() const
{
   return true;
}

/**
 * Add data collection element to proxy info structure
 */
void DataCollectionTarget::addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco, UINT32 primaryProxyId)
{
   info->msg->setField(info->baseInfoFieldId++, dco->getId());
   info->msg->setField(info->baseInfoFieldId++, static_cast<int16_t>(dco->getType()));
   info->msg->setField(info->baseInfoFieldId++, static_cast<int16_t>(dco->getDataSource()));
   info->msg->setField(info->baseInfoFieldId++, dco->getName());
   info->msg->setField(info->baseInfoFieldId++, dco->getEffectivePollingInterval());
   info->msg->setFieldFromTime(info->baseInfoFieldId++, dco->getLastPollTime());
   info->msg->setField(info->baseInfoFieldId++, m_guid);
   info->msg->setField(info->baseInfoFieldId++, dco->getSnmpPort());
   if (dco->getType() == DCO_TYPE_ITEM)
      info->msg->setField(info->baseInfoFieldId++, static_cast<const DCItem*>(dco)->getSnmpRawValueType());
   else
      info->msg->setField(info->baseInfoFieldId++, (INT16)0);
   info->msg->setField(info->baseInfoFieldId++, primaryProxyId);

   if (dco->getDataSource() == DS_SNMP_AGENT)
   {
      info->msg->setField(info->extraInfoFieldId++, static_cast<int16_t>(dco->getSnmpVersion()));
      if (dco->getType() == DCO_TYPE_TABLE)
      {
         info->extraInfoFieldId += 8;
         const ObjectArray<DCTableColumn> &columns = static_cast<const DCTable*>(dco)->getColumns();
         int count = std::min(columns.size(), 99);
         info->msg->setField(info->extraInfoFieldId++, count);
         for(int i = 0; i < count; i++)
         {
            columns.get(i)->fillMessage(info->msg, info->extraInfoFieldId);
            info->extraInfoFieldId += 10;
         }
         info->extraInfoFieldId += (99 - count) * 10;
      }
      else
      {
         info->extraInfoFieldId += 999;
      }
   }
   else
   {
      info->extraInfoFieldId += 1000;
   }

   if (dco->isAdvancedSchedule())
   {
      dco->fillSchedulingDataMessage(info->msg, info->extraInfoFieldId);
   }
   info->extraInfoFieldId += 100;

   info->count++;
}

/**
 * Add SNMP target to proxy info structure
 */
void DataCollectionTarget::addProxySnmpTarget(ProxyInfo *info, const Node *node)
{
   info->msg->setField(info->nodeInfoFieldId++, m_guid);
   info->msg->setField(info->nodeInfoFieldId++, node->getIpAddress());
   info->msg->setField(info->nodeInfoFieldId++, node->getSNMPVersion());
   info->msg->setField(info->nodeInfoFieldId++, node->getSNMPPort());
   SNMP_SecurityContext *snmpSecurity = node->getSnmpSecurityContext();
   info->msg->setField(info->nodeInfoFieldId++, (INT16)snmpSecurity->getAuthMethod());
   info->msg->setField(info->nodeInfoFieldId++, (INT16)snmpSecurity->getPrivMethod());
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getUser());
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getAuthPassword());
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getPrivPassword());
   delete snmpSecurity;
   info->nodeInfoFieldId += 41;
   info->nodeInfoCount++;
}

/**
 * Collect info for SNMP proxy and DCI source (proxy) nodes
 * Default implementation adds only agent based DCIs with source node set to requesting node
 */
void DataCollectionTarget::collectProxyInfo(ProxyInfo *info)
{
   if ((m_status == STATUS_UNMANAGED) || (m_state & DCSF_UNREACHABLE))
      return;

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco, 0);
      }
   }
   unlockDciAccess();
}

/**
 * Callback for collecting proxied SNMP DCIs
 */
void DataCollectionTarget::collectProxyInfoCallback(NetObj *object, void *data)
{
   static_cast<DataCollectionTarget*>(object)->collectProxyInfo(static_cast<ProxyInfo*>(data));
}

/**
 * Get effective source node for given data collection object
 */
uint32_t DataCollectionTarget::getEffectiveSourceNode(DCObject *dco)
{
   return dco->getSourceNode();
}

/**
 * Filter for selecting templates from objects
 */
static bool TemplateSelectionFilter(NetObj *object, void *userData)
{
   return (object->getObjectClass() == OBJECT_TEMPLATE) && !object->isDeleted() && static_cast<Template*>(object)->isAutoBindEnabled();
}

/**
 * Apply user templates
 */
void DataCollectionTarget::applyUserTemplates()
{
   if (IsShutdownInProgress())
      return;

   SharedObjectArray<NetObj> *templates = g_idxObjectById.getObjects(TemplateSelectionFilter);
   for(int i = 0; i < templates->size(); i++)
   {
      Template *pTemplate = (Template *)templates->get(i);
      AutoBindDecision decision = pTemplate->isApplicable(self());
      if (decision == AutoBindDecision_Bind)
      {
         if (!pTemplate->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::applyUserTemplates(): applying template %d \"%s\" to object %d \"%s\""),
                      pTemplate->getId(), pTemplate->getName(), m_id, m_name);
            pTemplate->applyToTarget(self());
            PostSystemEvent(EVENT_TEMPLATE_AUTOAPPLY, g_dwMgmtNode, "isis", m_id, m_name, pTemplate->getId(), pTemplate->getName());
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (pTemplate->isAutoUnbindEnabled() && pTemplate->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::applyUserTemplates(): removing template %d \"%s\" from object %d \"%s\""),
                      pTemplate->getId(), pTemplate->getName(), m_id, m_name);
            pTemplate->deleteChild(*this);
            deleteParent(*pTemplate);
            pTemplate->queueRemoveFromTarget(m_id, true);
            PostSystemEvent(EVENT_TEMPLATE_AUTOREMOVE, g_dwMgmtNode, "isis", m_id, m_name, pTemplate->getId(), pTemplate->getName());
         }
      }
   }
   delete templates;
}

/**
 * Filter for selecting containers from objects
 */
static bool ContainerSelectionFilter(NetObj *object, void *userData)
{
   return (object->getObjectClass() == OBJECT_CONTAINER) && !object->isDeleted() && ((Container *)object)->isAutoBindEnabled();
}

/**
 * Update container membership
 */
void DataCollectionTarget::updateContainerMembership()
{
   if (IsShutdownInProgress())
      return;

   SharedObjectArray<NetObj> *containers = g_idxObjectById.getObjects(ContainerSelectionFilter);
   for(int i = 0; i < containers->size(); i++)
   {
      Container *pContainer = (Container *)containers->get(i);
      AutoBindDecision decision = pContainer->isApplicable(self());
      if (decision == AutoBindDecision_Bind)
      {
         if (!pContainer->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::updateContainerMembership(): binding object %d \"%s\" to container %d \"%s\""),
                      m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->addChild(self());
            addParent(pContainer->self());
            PostSystemEvent(EVENT_CONTAINER_AUTOBIND, g_dwMgmtNode, "isis", m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->calculateCompoundStatus();
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (pContainer->isAutoUnbindEnabled() && pContainer->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::updateContainerMembership(): removing object %d \"%s\" from container %d \"%s\""),
                      m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->deleteChild(*this);
            deleteParent(*pContainer);
            PostSystemEvent(EVENT_CONTAINER_AUTOUNBIND, g_dwMgmtNode, "isis", m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->calculateCompoundStatus();
         }
      }
   }
   delete containers;
}

/**
 * Serialize object to JSON
 */
json_t *DataCollectionTarget::toJson()
{
   return super::toJson();
}

/**
 * Entry point for status poll worker thread
 */
void DataCollectionTarget::statusPollWorkerEntry(PollerInfo *poller)
{
   statusPollWorkerEntry(poller, nullptr, 0);
}

/**
 * Entry point for status poll worker thread
 */
void DataCollectionTarget::statusPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   statusPoll(poller, session, rqId);
   delete poller;
}

/**
 * Entry point for second level status poll (called by parent object)
 */
void DataCollectionTarget::statusPollPollerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->setStatus(_T("child poll"));
   statusPoll(poller, session, rqId);
}

/**
 * Perform status poll on this data collection target. Default implementation do nothing.
 */
void DataCollectionTarget::statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
}

/**
 * Entry point for configuration poll worker thread
 */
void DataCollectionTarget::configurationPollWorkerEntry(PollerInfo *poller)
{
   configurationPollWorkerEntry(poller, nullptr, 0);
}

/**
 * Entry point for configuration poll worker thread
 */
void DataCollectionTarget::configurationPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   poller->startObjectTransaction();
   configurationPoll(poller, session, rqId);
   poller->endObjectTransaction();
   delete poller;
}

/**
 * Perform configuration poll on this data collection target. Default implementation do nothing.
 */
void DataCollectionTarget::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
}

/**
 * Entry point for instance discovery poll worker thread
 */
void DataCollectionTarget::instanceDiscoveryPollWorkerEntry(PollerInfo *poller)
{
   instanceDiscoveryPollWorkerEntry(poller, nullptr, 0);
}

/**
 * Entry point for instance discovery poll worker thread
 */
void DataCollectionTarget::instanceDiscoveryPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 requestId)
{
   poller->startExecution();
   poller->startObjectTransaction();
   instanceDiscoveryPoll(poller, session, requestId);
   poller->endObjectTransaction();
   delete poller;
}

/**
 * Perform instance discovery poll on data collection target
 */
void DataCollectionTarget::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 requestId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_instancePollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(instance);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   sendPollerMsg(requestId, _T("Starting instance discovery poll for %s %s\r\n"), getObjectClassName(), m_name);
   DbgPrintf(4, _T("Starting instance discovery poll for %s %s (ID: %d)"), getObjectClassName(), m_name, m_id);

   // Check if DataCollectionTarget is marked as unreachable
   if (!(m_state & DCSF_UNREACHABLE))
   {
      poller->setStatus(_T("instance discovery"));
      doInstanceDiscovery(requestId);

      // Update time intervals in data collection objects
      updateDataCollectionTimeIntervals();

      // Execute hook script
      poller->setStatus(_T("hook"));
      executeHookScript(_T("InstancePoll"));
   }
   else
   {
      sendPollerMsg(requestId, POLLER_WARNING _T("%s is marked as unreachable, instance discovery poll aborted\r\n"), getObjectClassName());
      DbgPrintf(4, _T("%s is marked as unreachable, instance discovery poll aborted"), getObjectClassName());
   }

   // Finish instance discovery poll
   poller->setStatus(_T("cleanup"));
   pollerUnlock();
   DbgPrintf(4, _T("Finished instance discovery poll for %s %s (ID: %d)"), getObjectClassName(), m_name, m_id);
}

/**
 * Get list of instances for given data collection object. Default implementation always returns nullptr.
 */
StringMap *DataCollectionTarget::getInstanceList(DCObject *dco)
{
   return nullptr;
}

/**
 * Cancellation checkpoint for instance discovery loop
 */
#define INSTANCE_DISCOVERY_CANCELLATION_CHECKPOINT \
if (g_flags & AF_SHUTDOWN) \
{ \
   object->clearBusyFlag(); \
   for(i++; i < rootObjects.size(); i++) \
      rootObjects.get(i)->clearBusyFlag(); \
   delete instances; \
   changed = false; \
   break; \
}

/**
 * Do instance discovery
 */
void DataCollectionTarget::doInstanceDiscovery(UINT32 requestId)
{
   sendPollerMsg(requestId, _T("Running DCI instance discovery\r\n"));

   // collect instance discovery DCIs
   SharedObjectArray<DCObject> rootObjects;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      shared_ptr<DCObject> object = m_dcObjects->getShared(i);
      if (object->getInstanceDiscoveryMethod() != IDM_NONE)
      {
         object->setBusyFlag();
         rootObjects.add(object);
      }
   }
   unlockDciAccess();

   // process instance discovery DCIs
   // it should be done that way to prevent DCI list lock for long time
   bool changed = false;
   for(int i = 0; i < rootObjects.size(); i++)
   {
      DCObject *object = rootObjects.get(i);
      DbgPrintf(5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): Updating instances for instance discovery DCO %s [%d]"),
                m_name, m_id, object->getName().cstr(), object->getId());
      sendPollerMsg(requestId, _T("   Updating instances for %s [%d]\r\n"), object->getName().cstr(), object->getId());
      StringMap *instances = getInstanceList(object);
      INSTANCE_DISCOVERY_CANCELLATION_CHECKPOINT;
      if (instances != nullptr)
      {
         DbgPrintf(5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): read %d values"), m_name, m_id, instances->size());
         StringObjectMap<InstanceDiscoveryData> *filteredInstances = object->filterInstanceList(instances);
         INSTANCE_DISCOVERY_CANCELLATION_CHECKPOINT;
         if (updateInstances(object, filteredInstances, requestId))
            changed = true;
         delete filteredInstances;
         delete instances;
      }
      else
      {
         DbgPrintf(5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): failed to get instance list for DCO %s [%d]"),
                   m_name, m_id, object->getName().cstr(), object->getId());
         sendPollerMsg(requestId, POLLER_ERROR _T("      Failed to get instance list\r\n"));
      }
      object->clearBusyFlag();
   }

   if (changed)
   {
      onDataCollectionChange();

      lockProperties();
      setModified(MODIFY_DATA_COLLECTION);
      unlockProperties();
   }
}

/**
 * Callback for finding instance
 */
static EnumerationCallbackResult FindInstanceCallback(const TCHAR *key, const InstanceDiscoveryData *value, const TCHAR *data)
{
   return !_tcscmp(data, key) ? _STOP : _CONTINUE;
}

/**
 * Data for CreateInstanceDCI
 */
struct CreateInstanceDCOData
{
   DCObject *root;
   shared_ptr<DataCollectionTarget> object;
   UINT32 requestId;
};

/**
 * Callback for creating instance DCIs
 */
static EnumerationCallbackResult CreateInstanceDCI(const TCHAR *key, const InstanceDiscoveryData *value, CreateInstanceDCOData *data)
{
   auto object = data->object;
   auto root = data->root;

   DbgPrintf(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): creating new DCO for instance \"%s\""),
             object->getName(), object->getId(), root->getName().cstr(), root->getId(), key);
   object->sendPollerMsg(data->requestId, _T("      Creating new DCO for instance \"%s\"\r\n"), key);

   DCObject *dco = root->clone();

   dco->setTemplateId(object->getId(), root->getId());
   dco->setInstance(value->getInstance());
   dco->setInstanceDiscoveryMethod(IDM_NONE);
   dco->setInstanceDiscoveryData(key);
   dco->setInstanceFilter(nullptr);
   dco->expandInstance();
   dco->setRelatedObject(value->getRelatedObject());
   dco->changeBinding(CreateUniqueId(IDG_ITEM), object, false);
   object->addDCObject(dco, true);
   return _CONTINUE;
}

/**
 * Update instance DCIs created from instance discovery DCI
 */
bool DataCollectionTarget::updateInstances(DCObject *root, StringObjectMap<InstanceDiscoveryData> *instances, UINT32 requestId)
{
   bool changed = false;

   writeLockDciAccess();

   // Delete DCIs for missing instances and update existing
   IntegerArray<UINT32> deleteList;
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if ((object->getTemplateId() != m_id) || (object->getTemplateItemId() != root->getId()))
         continue;

      SharedString dcoInstance = object->getInstanceDiscoveryData();
      if (instances->forEach(FindInstanceCallback, dcoInstance.cstr()) == _STOP)
      {
         // found, remove value from instances
         nxlog_debug(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" found"),
                   m_name, m_id, root->getName().cstr(), root->getId(), dcoInstance.cstr());
         InstanceDiscoveryData *instanceObject = instances->get(dcoInstance);
         const TCHAR *name = instanceObject->getInstance();
         bool notify = false;
         if (_tcscmp(name, object->getInstance()))
         {
            object->setInstance(name);
            object->updateFromTemplate(root);
            changed = true;
            notify = true;
         }
         if (object->getInstanceGracePeriodStart() > 0)
         {
            object->setInstanceGracePeriodStart(0);
            object->setStatus(ITEM_STATUS_ACTIVE, false);
         }
         if(instanceObject->getRelatedObject() != object->getRelatedObject())
         {
            object->setRelatedObject(instanceObject->getRelatedObject());
            changed = true;
            notify = true;
         }
         instances->remove(dcoInstance);
         if (notify)
            NotifyClientsOnDCIUpdate(*this, object);
      }
      else
      {
         time_t retentionTime = ((object->getInstanceRetentionTime() != -1) ? object->getInstanceRetentionTime() : g_instanceRetentionTime) * 86400;

         if ((object->getInstanceGracePeriodStart() == 0) && (retentionTime > 0))
         {
            object->setInstanceGracePeriodStart(time(nullptr));
            object->setStatus(ITEM_STATUS_DISABLED, false);
            nxlog_debug(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, grace period started"),
                      m_name, m_id, root->getName().cstr(), root->getId(), dcoInstance.cstr());
            sendPollerMsg(requestId, _T("      Existing instance \"%s\" not found, grace period started\r\n"), dcoInstance.cstr());
            changed = true;
         }

         if ((retentionTime == 0) || ((time(nullptr) - object->getInstanceGracePeriodStart()) > retentionTime))
         {
            // not found, delete DCO
            nxlog_debug(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, instance DCO will be deleted"),
                      m_name, m_id, root->getName().cstr(), root->getId(), dcoInstance.cstr());
            sendPollerMsg(requestId, _T("      Existing instance \"%s\" not found and will be deleted\r\n"), dcoInstance.cstr());
            deleteList.add(object->getId());
            changed = true;
         }
      }
   }

   for(int i = 0; i < deleteList.size(); i++)
      deleteDCObject(deleteList.get(i), false, 0);

   // Create new instances
   if (instances->size() > 0)
   {
      CreateInstanceDCOData data;
      data.root = root;
      data.object = self();
      data.requestId = requestId;
      instances->forEach(CreateInstanceDCI, &data);
      changed = true;
   }

   unlockDciAccess();
   return changed;
}

/**
 * Get last (current) DCI values.
 */
UINT32 DataCollectionTarget::getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, UINT32 userId)
{
   readLockDciAccess();

   UINT32 dwId = VID_DCI_VALUES_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if ((object->hasValue() || includeNoValueObjects) &&
          (!objectTooltipOnly || object->isShowOnObjectTooltip()) &&
          (!overviewOnly || object->isShowInObjectOverview()) &&
          object->hasAccess(userId))
      {
         if (object->getType() == DCO_TYPE_ITEM)
         {
            ((DCItem *)object)->fillLastValueMessage(msg, dwId);
            dwId += 50;
            dwCount++;
         }
         else if (object->getType() == DCO_TYPE_TABLE)
         {
            ((DCTable *)object)->fillLastValueSummaryMessage(msg, dwId);
            dwId += 50;
            dwCount++;
         }
      }
   }
   msg->setField(VID_NUM_ITEMS, dwCount);

   unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Hook for data collection load
 */
void DataCollectionTarget::onDataCollectionLoad()
{
   super::onDataCollectionLoad();
   calculateProxyLoad();
}

/**
 * Hook for data collection change
 */
void DataCollectionTarget::onDataCollectionChange()
{
   super::onDataCollectionChange();
   calculateProxyLoad();
}

/**
 * Calculate proxy load factor
 */
void DataCollectionTarget::calculateProxyLoad()
{
   double loadFactor = 0;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if ((object->getDataSource() == DS_SNMP_AGENT) && (object->getStatus() == ITEM_STATUS_ACTIVE))
      {
         if (object->isAdvancedSchedule())
            loadFactor += 12;  // assume 5 minutes interval for custom schedule
         else
            loadFactor += 3600.0 / static_cast<double>(object->getEffectivePollingInterval());
      }
   }
   unlockDciAccess();

   lockProperties();
   m_proxyLoadFactor = loadFactor;
   unlockProperties();
}

/**
 * Reset poll timers
 */
void DataCollectionTarget::resetPollTimers()
{
   m_statusPollState.resetTimer();
   m_configurationPollState.resetTimer();
   m_instancePollState.resetTimer();
}

/**
 * Get list of template type parent objects for NXSL script
 */
NXSL_Array *DataCollectionTarget::getTemplatesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *parents = new NXSL_Array(vm);
   int index = 0;

   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if ((object->getObjectClass() == OBJECT_TEMPLATE) && object->isTrustedNode(m_id))
      {
         parents->set(index++, object->createNXSLObject(vm));
      }
   }
   unlockParentList();

   return parents;
}

/**
 * Get cache memory usage
 */
UINT64 DataCollectionTarget::getCacheMemoryUsage()
{
   UINT64 cacheSize = 0;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if (object->getType() == DCO_TYPE_ITEM)
      {
         cacheSize += static_cast<DCItem*>(object)->getCacheMemoryUsage();
      }
   }
   unlockDciAccess();
   return cacheSize;
}

/**
 * Get effective web service proxy for this node
 */
uint32_t DataCollectionTarget::getEffectiveWebServiceProxy()
{
   uint32_t webServiceProxy = 0;
   int32_t zoneUIN = getZoneUIN();
   if (IsZoningEnabled() && (webServiceProxy == 0) && (zoneUIN != 0))
   {
      // Use zone default proxy if set
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         webServiceProxy = zone->isProxyNode(m_id) ? m_id : zone->getProxyNodeId(this);
      }
   }
   return (webServiceProxy != 0) ? webServiceProxy : g_dwMgmtNode;
}
