/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.webui.core;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.netxms.api.client.Session;
import org.netxms.client.NXCSession;
import org.netxms.webui.core.dialogs.LoginForm;
import org.netxms.webui.core.dialogs.PasswordExpiredDialog;
import org.netxms.webui.tools.RWTHelper;

/**
 * Configures the initial size and appearance of a workbench window.
 */
public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{

	public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		super(configurer);
	}

	public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
	{
		return new ApplicationActionBarAdvisor(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#preWindowOpen()
	 */
	@Override
	public void preWindowOpen()
	{
		doLogin();
		
		IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
		configurer.setInitialSize(new Point(400, 300));
		configurer.setShowCoolBar(false);
		configurer.setShowStatusLine(false);
		configurer.setTitle("NetXMS Management Console");
		configurer.setShellStyle(SWT.NO_TRIM);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowCreate()
	 */
	@Override
	public void postWindowCreate()
	{
		super.postWindowCreate();
		getWindowConfigurer().getWindow().getShell().setMaximized(true);
	}

	/**
	 * Show login dialog and perform login
	 */
	private void doLogin()
	{
		boolean success = false;

		LoginForm loginDialog;
		do
		{
			loginDialog = new LoginForm(null);
			if (loginDialog.open() != Window.OK)
				continue;

			try
			{
				// TODO: read server address from app settings
				LoginJob job = new LoginJob("127.0.0.1", loginDialog.getLogin(), //$NON-NLS-1$
				                            loginDialog.getPassword(), Display.getCurrent());

				// TODO: implement login on non-UI thread
				new ProgressMonitorDialog(null).run(false, false, job);
				success = true;
			}
			catch(InvocationTargetException e)
			{
				MessageDialog.openError(null, "Connection Error", e.getCause().getLocalizedMessage());
			}
			catch(Exception e)
			{
				e.printStackTrace();
				MessageDialog.openError(null, "Exception", e.toString());
			}
		} while(!success);

		if (success)
		{
			// Suggest user to change password if it is expired
			final Session session = (Session)RWT.getSessionStore().getAttribute("netxms.session");
			if (session.isPasswordExpired())
			{
				final PasswordExpiredDialog dlg = new PasswordExpiredDialog(null);
				if (dlg.open() == Window.OK)
				{
					final String currentPassword = loginDialog.getPassword();
					final Display display = Display.getCurrent();
					IRunnableWithProgress job = new IRunnableWithProgress() {
						@Override
						public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
						{
							try
							{
								NXCSession session = (NXCSession)RWTHelper.getSessionAttribute(display, "netxms.session");
								session.setUserPassword(session.getUserId(), dlg.getPassword(), currentPassword);
							}
							catch(Exception e)
							{
								throw new InvocationTargetException(e);
							}
							finally
							{
								monitor.done();
							}
						}
					};
					try
					{
						new ProgressMonitorDialog(null).run(true, true, job);
						MessageDialog.openInformation(null, "Information", "Password changed successfully");
					}
					catch(InvocationTargetException e)
					{
						MessageDialog.openError(null, "Error", "Cannot change password: " + e.getCause().getLocalizedMessage());
					}
					catch(InterruptedException e)
					{
						MessageDialog.openError(null, "Exception", e.toString());
					}
				}
			}
		}
	}
}
