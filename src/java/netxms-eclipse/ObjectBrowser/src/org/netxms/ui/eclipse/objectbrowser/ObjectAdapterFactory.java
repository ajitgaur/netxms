/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import java.util.Iterator;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class ObjectAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("unchecked")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if (adapterType == IWorkbenchAdapter.class)
		{
			if (adaptableObject instanceof NXCObject)
			{
				return new IWorkbenchAdapter() {
					@Override
					public Object[] getChildren(Object o)
					{
						return ((NXCObject)o).getChildsAsArray();
					}

					@Override
					public ImageDescriptor getImageDescriptor(Object object)
					{
						switch(((NXCObject)object).getObjectClass())
						{
							case NXCObject.OBJECT_NETWORK:
								return Activator.getImageDescriptor("icons/network.png");
							case NXCObject.OBJECT_SERVICEROOT:
								return Activator.getImageDescriptor("icons/service_root.png");
							case NXCObject.OBJECT_CONTAINER:
								return Activator.getImageDescriptor("icons/container.png");
							case NXCObject.OBJECT_SUBNET:
								return Activator.getImageDescriptor("icons/subnet.png");
							case NXCObject.OBJECT_NODE:
								return Activator.getImageDescriptor("icons/node.png");
							case NXCObject.OBJECT_INTERFACE:
								return Activator.getImageDescriptor("icons/interface.png");
							case NXCObject.OBJECT_CONDITION:
								return Activator.getImageDescriptor("icons/condition.png");
							case NXCObject.OBJECT_TEMPLATEROOT:
								return Activator.getImageDescriptor("icons/template_root.png");
							case NXCObject.OBJECT_TEMPLATEGROUP:
								return Activator.getImageDescriptor("icons/template_group.png");
							case NXCObject.OBJECT_TEMPLATE:
								return Activator.getImageDescriptor("icons/template.png");
							default:
								return null;
						}
					}

					@Override
					public String getLabel(Object o)
					{
						return ((NXCObject)o).getObjectName();
					}

					@Override
					public Object getParent(Object o)
					{
						NXCSession session = NXMCSharedData.getInstance().getSession();
						if (session != null)
						{
							Iterator<Long> it = ((NXCObject)o).getParents();
							return it.hasNext() ? session.findObjectById(it.next()) : null;
						}
						return null;
					}
				};
			}
		}
		return null;
	}
}
