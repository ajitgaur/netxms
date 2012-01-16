/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.webui.core.dialogs;

import java.util.Arrays;
import java.util.HashSet;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Monitor;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.webui.core.Activator;

/**
 * Login dialog
 */
public class LoginDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	private ImageDescriptor loginImage;
	private Combo comboServer;
	private Text textLogin;
	private Text textPassword;
	private String password;
	private boolean isOk = false;
	
	public LoginDialog(Shell parentShell)
	{
		super(parentShell);
		loginImage = Activator.getImageDescriptor("icons/login.png"); //$NON-NLS-1$
	}
	
	@Override
	protected void configureShell(Shell newShell)
	{
      super.configureShell(newShell);
      newShell.setText("Connect To Server");
      
      // Center dialog on screen
      // We don't have main window at this moment, so use
      // monitor data to determine right position
      Monitor [] ma = newShell.getDisplay().getMonitors();
      if (ma != null)
      	newShell.setLocation((ma[0].getClientArea().width - newShell.getSize().x) / 2, (ma[0].getClientArea().height - newShell.getSize().y) / 2);   
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) 
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.numColumns = 2;
      dialogLayout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogLayout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogLayout.horizontalSpacing = 0;
      dialogArea.setLayout(dialogLayout);
      
      // Header image
      Label label = new Label(dialogArea, SWT.NONE);
      label.setBackground(new Color(dialogArea.getDisplay(), 36, 66, 90));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      label.setLayoutData(gd);
      
      label = new Label(dialogArea, SWT.NONE);
      label.setImage(loginImage.createImage());
      label.addDisposeListener(
      		new DisposeListener() {
				private static final long serialVersionUID = 1L;

				public void widgetDisposed(DisposeEvent event)
      			{
      				((Label)event.widget).getImage().dispose();
      			}
      		});
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      label.setLayoutData(gd);
      
      // Connection
      Group groupConn = new Group(dialogArea, SWT.NONE);
      groupConn.setText("Connection");
      GridLayout gridLayout = new GridLayout(2, false);
      gridLayout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      gridLayout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      groupConn.setLayout(gridLayout);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      groupConn.setLayoutData(gd);

      label = new Label(groupConn, SWT.NONE);
      label.setText("Server");
      comboServer = new Combo(groupConn, SWT.DROP_DOWN);
      String[] items = settings.getArray("Connect.ServerHistory"); //$NON-NLS-1$
      if (items != null)
      	comboServer.setItems(items);
      String text = settings.get("Connect.Server"); //$NON-NLS-1$
      if (text != null)
      	comboServer.setText(text);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      comboServer.setLayoutData(gridData);
      
      label = new Label(groupConn, SWT.NONE);
      label.setText("Login");
      textLogin = new Text(groupConn, SWT.SINGLE | SWT.BORDER);
      text = settings.get("Connect.Login"); //$NON-NLS-1$
      if (text != null)
      	textLogin.setText(text);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      textLogin.setLayoutData(gridData);
      
      label = new Label(groupConn, SWT.NONE);
      label.setText("Password");
      textPassword = new Text(groupConn, SWT.SINGLE | SWT.PASSWORD | SWT.BORDER);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      textPassword.setLayoutData(gridData);
      
		if (comboServer.getText().isEmpty())
			comboServer.setFocus();
		else if (textLogin.getText().isEmpty())
			textLogin.setFocus();
		else
			textPassword.setFocus();
		
      return dialogArea;
   }
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() 
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      
      HashSet<String> items = new HashSet<String>();
      items.addAll(Arrays.asList(comboServer.getItems()));
      items.add(comboServer.getText());
   
      settings.put("Connect.Server", comboServer.getText()); //$NON-NLS-1$
      settings.put("Connect.ServerHistory", items.toArray(new String[items.size()])); //$NON-NLS-1$
      settings.put("Connect.Login", textLogin.getText()); //$NON-NLS-1$
      
      password = textPassword.getText();
  
      isOk = true;
      
      super.okPressed();
   }

	/**
	 * @return the password
	 */
	public String getPassword()
	{
		return password;
	}

	
	/**
	 * @return true if dialog was closed with OK button, false otherwise
	 */
	public boolean isOk()
	{
		return isOk;
	}
}
