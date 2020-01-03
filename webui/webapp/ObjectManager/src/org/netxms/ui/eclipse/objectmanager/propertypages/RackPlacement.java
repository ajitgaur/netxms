/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.RackElement;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * "Rack" property page for NetXMS object
 */
public class RackPlacement extends PropertyPage
{
   private final static String[] ORIENTATION = { "Fill", "Front", "Rear" };
   
	private RackElement object;
	private ObjectSelector rackSelector;
	private ImageSelector rackImageFrontSelector;
   private ImageSelector rackImageRearSelector;
	private LabeledSpinner rackHeight;
	private LabeledSpinner rackPosition;
	private Combo rackOrientation;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (RackElement)getElement().getAdapter(RackElement.class);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 3;
      dialogArea.setLayout(layout);

      rackSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      rackSelector.setLabel(Messages.get().RackPlacement_Rack);
      rackSelector.setObjectClass(Rack.class);
      rackSelector.setObjectId(object.getRackId());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 3;
		rackSelector.setLayoutData(gd);
		
		rackImageFrontSelector = new ImageSelector(dialogArea, SWT.NONE);
		rackImageFrontSelector.setLabel("Rack front image");
		rackImageFrontSelector.setImageGuid(object.getFrontRackImage(), false);
		rackImageFrontSelector.setEnabled(object.getRackOrientation() == RackOrientation.FRONT ||
		                                  object.getRackOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      rackImageFrontSelector.setLayoutData(gd);
      
      rackImageRearSelector = new ImageSelector(dialogArea, SWT.NONE);
      rackImageRearSelector.setLabel("Rack rear image");
      rackImageRearSelector.setImageGuid(object.getRearRackImage(), false);
      rackImageRearSelector.setEnabled(object.getRackOrientation() == RackOrientation.REAR ||
                                       object.getRackOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      rackImageRearSelector.setLayoutData(gd);
      
      rackPosition = new LabeledSpinner(dialogArea, SWT.NONE);
      rackPosition.setLabel(Messages.get().RackPlacement_Position);
      rackPosition.setRange(1, 50);
      rackPosition.setSelection(object.getRackPosition());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackPosition.setLayoutData(gd);
		
      rackHeight = new LabeledSpinner(dialogArea, SWT.NONE);
      rackHeight.setLabel(Messages.get().RackPlacement_Height);
      rackHeight.setRange(1, 50);
      rackHeight.setSelection(object.getRackHeight());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackHeight.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackOrientation = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Orientation", gd);
      rackOrientation.setItems(ORIENTATION);
      rackOrientation.setText(ORIENTATION[object.getRackOrientation().getValue()]);
      rackOrientation.addSelectionListener(new SelectionListener() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (RackOrientation.getByValue(rackOrientation.getSelectionIndex()) == RackOrientation.FRONT)
            {
               rackImageRearSelector.setEnabled(false);
               rackImageFrontSelector.setEnabled(true);
            }
            else if (RackOrientation.getByValue(rackOrientation.getSelectionIndex()) == RackOrientation.REAR)
            {
               rackImageRearSelector.setEnabled(true);
               rackImageFrontSelector.setEnabled(false);
            }
            else
            {
               rackImageRearSelector.setEnabled(true);
               rackImageFrontSelector.setEnabled(true);
            }
         }         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setRackPlacement(rackSelector.getObjectId(), rackImageFrontSelector.getImageGuid(), rackImageRearSelector.getImageGuid(), (short)rackPosition.getSelection(), (short)rackHeight.getSelection(),
		                    RackOrientation.getByValue(rackOrientation.getSelectionIndex()));
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(String.format(Messages.get().RackPlacement_UpdatingRackPlacement, object.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
			   session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Filed to update rack placement";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							RackPlacement.this.setValid(true);
						}
					});
				}
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		rackSelector.setObjectId(0);
		rackImageFrontSelector.setImageGuid(NXCommon.EMPTY_GUID, true);
      rackImageRearSelector.setImageGuid(NXCommon.EMPTY_GUID, true);
		rackPosition.setSelection(1);
		rackHeight.setSelection(1);
		rackOrientation.select(0);
	}
}