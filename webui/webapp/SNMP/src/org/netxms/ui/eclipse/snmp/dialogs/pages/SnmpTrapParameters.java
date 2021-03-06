/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp.dialogs.pages;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.client.snmp.SnmpTrapParameterMapping;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.ParamMappingEditDialog;
import org.netxms.ui.eclipse.snmp.dialogs.helpers.ParamMappingLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Parameters" property page for SNMP trap configuration
 */
public class SnmpTrapParameters extends PreferencePage
{
   private static final String PARAMLIST_TABLE_SETTINGS = "TrapConfigurationDialog.ParamList"; //$NON-NLS-1$
   
   private SnmpTrap trap;
   private List<SnmpTrapParameterMapping> pmap;
   private TableViewer paramList;
   private Button buttonAdd;
   private Button buttonEdit;
   private Button buttonDelete;
   private Button buttonUp;
   private Button buttonDown;
   
   /**
    * Create page
    * 
    * @param trap SNMP trap object to edit
    */
   public SnmpTrapParameters(SnmpTrap trap)
   {
      super("Parameters");
      noDefaultAndApplyButton();
      this.trap = trap;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      paramList = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 300;
      paramList.getTable().setLayoutData(gd);
      setupParameterList();
      
      Composite buttonArea = new Composite(dialogArea, SWT.NONE);
      RowLayout btnLayout = new RowLayout();
      btnLayout.type = SWT.VERTICAL;
      btnLayout.marginBottom = 0;
      btnLayout.marginLeft = 0;
      btnLayout.marginRight = 0;
      btnLayout.marginTop = 0;
      btnLayout.fill = true;
      btnLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonArea.setLayout(btnLayout);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      buttonArea.setLayoutData(gd);
      
      buttonAdd = new Button(buttonArea, SWT.PUSH);
      buttonAdd.setText(Messages.get().TrapConfigurationDialog_Add);
      buttonAdd.setLayoutData(new RowData(WidgetHelper.BUTTON_WIDTH_HINT, SWT.DEFAULT));
      buttonAdd.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addParameter();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      buttonEdit = new Button(buttonArea, SWT.PUSH);
      buttonEdit.setText(Messages.get().TrapConfigurationDialog_Edit);
      buttonEdit.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editParameter();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      buttonEdit.setEnabled(false);
      
      buttonDelete = new Button(buttonArea, SWT.PUSH);
      buttonDelete.setText(Messages.get().TrapConfigurationDialog_Delete);
      buttonDelete.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteParameters();
         }
      });
      buttonDelete.setEnabled(false);
      
      buttonUp = new Button(buttonArea, SWT.PUSH);
      buttonUp.setText(Messages.get().TrapConfigurationDialog_MoveUp);
      
      buttonDown = new Button(buttonArea, SWT.PUSH);
      buttonDown.setText(Messages.get().TrapConfigurationDialog_MoveDown);
      
      return dialogArea;
   }

   /**
    * Add new parameter mapping
    */
   private void addParameter()
   {
      SnmpTrapParameterMapping pm = new SnmpTrapParameterMapping(new SnmpObjectId());
      ParamMappingEditDialog dlg = new ParamMappingEditDialog(getShell(), pm);
      if (dlg.open() == Window.OK)
      {
         pmap.add(pm);
         paramList.setInput(pmap.toArray());
         paramList.setSelection(new StructuredSelection(pm));
      }
   }
   
   /**
    * Edit currently selected parameter mapping
    */
   private void editParameter()
   {
      IStructuredSelection selection = (IStructuredSelection)paramList.getSelection();
      if (selection.size() != 1)
         return;
      
      SnmpTrapParameterMapping pm = (SnmpTrapParameterMapping)selection.getFirstElement();
      ParamMappingEditDialog dlg = new ParamMappingEditDialog(getShell(), pm);
      if (dlg.open() == Window.OK)
      {
         paramList.update(pm, null);
      }
   }

   /**
    * Delete selected parameters
    */
   @SuppressWarnings("unchecked")
   private void deleteParameters()
   {
      IStructuredSelection selection = (IStructuredSelection)paramList.getSelection();
      if (selection.isEmpty())
         return;
      
      Iterator<SnmpTrapParameterMapping> it = selection.iterator();
      while(it.hasNext())
         pmap.remove(it.next());
      
      paramList.setInput(pmap.toArray());
   }

   /**
    * Setup parameter mapping list
    */
   private void setupParameterList()
   {
      Table table = paramList.getTable();
      table.setHeaderVisible(true);
      
      TableColumn tc = new TableColumn(table, SWT.LEFT);
      tc.setText(Messages.get().TrapConfigurationDialog_Number);
      tc.setWidth(90);
      
      tc = new TableColumn(table, SWT.LEFT);
      tc.setText(Messages.get().TrapConfigurationDialog_Parameter);
      tc.setWidth(200);
      
      pmap = new ArrayList<SnmpTrapParameterMapping>(trap.getParameterMapping());
      
      paramList.setContentProvider(new ArrayContentProvider());
      paramList.setLabelProvider(new ParamMappingLabelProvider(pmap));
      paramList.setInput(pmap.toArray());
      
      WidgetHelper.restoreColumnSettings(table, Activator.getDefault().getDialogSettings(), PARAMLIST_TABLE_SETTINGS);
      
      paramList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editParameter();
         }
      });

      paramList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = paramList.getStructuredSelection();
            buttonEdit.setEnabled(selection.size() == 1);
            buttonDelete.setEnabled(!selection.isEmpty());
         }
      });
   }
   
   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      WidgetHelper.saveColumnSettings(paramList.getTable(), Activator.getDefault().getDialogSettings(), PARAMLIST_TABLE_SETTINGS);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performCancel()
    */
   @Override
   public boolean performCancel()
   {
      if (isControlCreated())
         saveSettings();
      return super.performCancel();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      if (isControlCreated())
      {
         trap.getParameterMapping().clear();
         trap.getParameterMapping().addAll(pmap);
         saveSettings();
      }
      return super.performOk();
   }
}
