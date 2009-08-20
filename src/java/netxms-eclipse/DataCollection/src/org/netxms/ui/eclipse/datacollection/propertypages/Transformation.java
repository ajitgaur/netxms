/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class Transformation extends PropertyPage
{
	private DataCollectionItem dci;
	private Combo deltaCalculation;
	private Text transformationScript;
	private Button testScriptButton;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		dci = (DataCollectionItem)getElement().getAdapter(DataCollectionItem.class);
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      deltaCalculation = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, "Step 1 - delta calculation",
                                                         WidgetHelper.DEFAULT_LAYOUT_DATA);
      deltaCalculation.add("None (keep original value)");
      deltaCalculation.add("Simple delta");
      deltaCalculation.add("Average delta per second");
      deltaCalculation.add("Average delta per minute");
      deltaCalculation.select(dci.getDeltaCalculation());
     
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 0;
      gd.heightHint = 0;
      transformationScript = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL,
                                                            "Step 2 - transformation script", dci.getTransformationScript(), gd);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      transformationScript.setLayoutData(gd);
      
      testScriptButton = new Button(transformationScript.getParent(), SWT.PUSH);
      testScriptButton.setText("&Test...");   
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      testScriptButton.setLayoutData(gd);
		
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
		
		dci.setDeltaCalculation(deltaCalculation.getSelectionIndex());
		dci.setTransformationScript(transformationScript.getText());
		new Job("Update transformation settings for DCI " + dci.getId()) {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					dci.getOwner().modifyItem(dci);
					new UIJob("Update data collection item list") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							((TableViewer)dci.getOwner().getUserData()).update(dci, null);
							return Status.OK_STATUS;
						}
					}.schedule();
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot update transformation settings: " + e.getMessage(), null);
				}

				if (isApply)
				{
					new UIJob("Update \"Transformation\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							Transformation.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}

				return status;
			}
		}.schedule();
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
		deltaCalculation.select(DataCollectionItem.DELTA_NONE);
		transformationScript.setText("");
	}
}
