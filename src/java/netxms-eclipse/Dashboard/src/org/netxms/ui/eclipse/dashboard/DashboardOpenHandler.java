/**
 * 
 */
package org.netxms.ui.eclipse.dashboard;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.views.DashboardView;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler;

/**
 * Open handler for dashboard objects
 */
public class DashboardOpenHandler implements ObjectOpenHandler
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler#openObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean openObject(GenericObject object)
	{
		if (!(object instanceof Dashboard))
			return false;
		
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			window.getActivePage().showView(DashboardView.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialog.openError(window.getShell(), Messages.OpenDashboard_Error, Messages.OpenDashboard_ErrorText + e.getMessage());
		}
		return true;
	}
}
