/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.websvc.handlers;

import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public class GrafanaDataCollection extends AbstractHandler
{
   private List<AbstractObject> objects;
   private Logger log = LoggerFactory.getLogger(GrafanaDataCollection.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      if (!getSession().isObjectsSynchronized())
         getSession().syncObjects();

      objects = getSession().getAllObjects();
      if (query.containsKey("targets"))
      {
         return getGraphData(query);
      }
      else if (query.containsKey("target"))
      {
         return getDciList(query);
      }
      
      return getNodeList();     
   }
   
   /**
    * Get query data
    * 
    * @param query
    * @return data
    * @throws Exception
    */
   private JsonArray getGraphData(Map<String, String> query) throws Exception
   {
      JsonParser parser = new JsonParser();
      JsonElement element = parser.parse(query.get("targets"));
      if (!element.isJsonArray())
         return new JsonArray();
      
      JsonArray targets = element.getAsJsonArray();
      
      DateFormat format = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSX");
      Date from = format.parse(query.get("from").substring(1, query.get("from").length()-1));
      Date to = format.parse(query.get("to").substring(1, query.get("to").length()-1));

      JsonArray result = new JsonArray();
      for(JsonElement e : targets)
      {
         if (!e.getAsJsonObject().has("dciTarget") || !e.getAsJsonObject().has("dci"))
            continue;

         JsonObject dciTarget = e.getAsJsonObject().getAsJsonObject("dciTarget");
         JsonObject dci = e.getAsJsonObject().getAsJsonObject("dci");

         Long dciTargetId = Long.parseLong(dciTarget.get("id").getAsString());
         Long dciId = Long.parseLong(dci.get("id").getAsString());

         String dciName = dci.get("name").getAsString();
         if (dciId == 0)
         {
            String dciTargetName = dciTarget.get("name").getAsString();

            Boolean searchByName = e.getAsJsonObject().has("searchByName") ?
                  e.getAsJsonObject().get("searchByName").getAsBoolean() :
                  false;

            if (dciTargetName.startsWith("/") && dciTargetName.endsWith("/"))
            {
               dciTargetName = dciTargetName.substring(1, dciTargetName.length() - 1);
            }
            if (dciTargetName.startsWith("/") && dciTargetName.endsWith("/"))
            {
               dciName = dciName.substring(1, dciName.length() - 1);
            }

            List<DciValue> values = getSession()
                  .findMatchingDCI(dciTargetId, dciTargetName, dciName, searchByName ? NXCSession.DCI_RES_SEARCH_NAME : 0);

            for(DciValue v : values)
            {
               result.add(fillGraphData(v.getNodeId(), v.getId(), v.getDescription(), from, to));
            }
         }
         else
         {
            String legend = e.getAsJsonObject().get("legend").getAsString().isEmpty() ? dciName : e.getAsJsonObject().get("legend").getAsString();
            result.add(fillGraphData(dciTargetId, dciId, legend, from, to));
         }
      }
      return result;
   }

   /**
    * Fill graph data
    *
    * @param objectId
    * @param dciId
    * @param legend
    * @param from
    * @param to
    * @return
    * @throws IOException
    * @throws NXCException
    */
   private JsonObject fillGraphData(Long objectId, Long dciId, String legend, Date from, Date to) throws IOException, NXCException
   {
      DciData data = getSession().getCollectedData(objectId, dciId, from, to, 0, HistoricalDataType.PROCESSED);

      JsonObject root = new JsonObject();
      JsonArray datapoints = new JsonArray();
      DciDataRow[] values = data.getValues();
      for(int i = values.length - 1; i >= 0; i--)
      {
         DciDataRow r = values[i];
         JsonArray datapoint = new JsonArray();
         datapoint.add(r.getValueAsDouble());
         datapoint.add(r.getTimestamp().getTime());
         datapoints.add(datapoint);
      }

      root.addProperty("target", legend);
      root.add("datapoints", datapoints);

      return root;
   }
   
   /**
    * Get list of nodes
    * 
    * @return
    */
   private Map<Long, String> getNodeList()
   {
      Map<Long, String> result = new HashMap<Long, String>();
      for(AbstractObject o : objects)
      {
         if (o instanceof DataCollectionTarget)
            result.put(o.getObjectId(), o.getObjectName());
      }
      return result;
   }
   
   /**
    * Get list of dci`s for a node
    * 
    * @param query
    * @return dci list
    */
   private Map<Long, String> getDciList(Map<String, String> query) throws Exception
   {
      Map<Long, String> result = new HashMap<Long, String>();

      if (!query.containsKey("target"))
         return result;

      JsonParser parser = new JsonParser();
      JsonElement element = parser.parse(query.get("target"));

      try
      {
         Long id = element.getAsLong();
         DciValue[] values = getSession().getLastValues(id);

         for(DciValue v : values)
            result.put(v.getId(), v.getDescription());
      }
      catch (NXCException e)
      {
         log.debug("DCI not found");
      }

      return result;
   }
}
