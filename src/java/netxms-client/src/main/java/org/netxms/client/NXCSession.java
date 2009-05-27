/**
 *
 */
package org.netxms.client;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPDataInputStream;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.base.NXCPMsgWaitQueue;
import org.netxms.base.NetXMSConst;

/**
 * @author victor
 */
public class NXCSession
{
	// Various public constants
	public static final int DEFAULT_CONN_PORT = 4701;
	public static final int CLIENT_PROTOCOL_VERSION = 20;

	// Authentication types
	public static final int AUTH_TYPE_PASSWORD = 0;
	public static final int AUTH_TYPE_CERTIFICATE = 1;

	// Notification channels
	public static final int CHANNEL_EVENTS = 0x0001;
	public static final int CHANNEL_SYSLOG = 0x0002;
	public static final int CHANNEL_ALARMS = 0x0004;
	public static final int CHANNEL_OBJECTS = 0x0008;
	public static final int CHANNEL_SNMP_TRAPS = 0x0010;
	public static final int CHANNEL_AUDIT_LOG = 0x0020;
	public static final int CHANNEL_SITUATIONS = 0x0040;

	// Request completion codes (RCC)
	public static final int RCC_SUCCESS = 0;
	public static final int RCC_COMPONENT_LOCKED = 1;
	public static final int RCC_ACCESS_DENIED = 2;
	public static final int RCC_INVALID_REQUEST = 3;
	public static final int RCC_TIMEOUT = 4;
	public static final int RCC_OUT_OF_STATE_REQUEST = 5;
	public static final int RCC_DB_FAILURE = 6;
	public static final int RCC_INVALID_OBJECT_ID = 7;
	public static final int RCC_ALREADY_EXIST = 8;
	public static final int RCC_COMM_FAILURE = 9;
	public static final int RCC_SYSTEM_FAILURE = 10;
	public static final int RCC_INVALID_USER_ID = 11;
	public static final int RCC_INVALID_ARGUMENT = 12;
	public static final int RCC_DUPLICATE_DCI = 13;
	public static final int RCC_INVALID_DCI_ID = 14;
	public static final int RCC_OUT_OF_MEMORY = 15;
	public static final int RCC_IO_ERROR = 16;
	public static final int RCC_INCOMPATIBLE_OPERATION = 17;
	public static final int RCC_OBJECT_CREATION_FAILED = 18;
	public static final int RCC_OBJECT_LOOP = 19;
	public static final int RCC_INVALID_OBJECT_NAME = 20;
	public static final int RCC_INVALID_ALARM_ID = 21;
	public static final int RCC_INVALID_ACTION_ID = 22;
	public static final int RCC_OPERATION_IN_PROGRESS = 23;
	public static final int RCC_DCI_COPY_ERRORS = 24;
	public static final int RCC_INVALID_EVENT_CODE = 25;
	public static final int RCC_NO_WOL_INTERFACES = 26;
	public static final int RCC_NO_MAC_ADDRESS = 27;
	public static final int RCC_NOT_IMPLEMENTED = 28;
	public static final int RCC_INVALID_TRAP_ID = 29;
	public static final int RCC_DCI_NOT_SUPPORTED = 30;
	public static final int RCC_VERSION_MISMATCH = 31;
	public static final int RCC_NPI_PARSE_ERROR = 32;
	public static final int RCC_DUPLICATE_PACKAGE = 33;
	public static final int RCC_PACKAGE_FILE_EXIST = 34;
	public static final int RCC_RESOURCE_BUSY = 35;
	public static final int RCC_INVALID_PACKAGE_ID = 36;
	public static final int RCC_INVALID_IP_ADDR = 37;
	public static final int RCC_ACTION_IN_USE = 38;
	public static final int RCC_VARIABLE_NOT_FOUND = 39;
	public static final int RCC_BAD_PROTOCOL = 40;
	public static final int RCC_ADDRESS_IN_USE = 41;
	public static final int RCC_NO_CIPHERS = 42;
	public static final int RCC_INVALID_PUBLIC_KEY = 43;
	public static final int RCC_INVALID_SESSION_KEY = 44;
	public static final int RCC_NO_ENCRYPTION_SUPPORT = 45;
	public static final int RCC_INTERNAL_ERROR = 46;
	public static final int RCC_EXEC_FAILED = 47;
	public static final int RCC_INVALID_TOOL_ID = 48;
	public static final int RCC_SNMP_ERROR = 49;
	public static final int RCC_BAD_REGEXP = 50;
	public static final int RCC_UNKNOWN_PARAMETER = 51;
	public static final int RCC_FILE_IO_ERROR = 52;
	public static final int RCC_CORRUPTED_MIB_FILE = 53;
	public static final int RCC_TRANSFER_IN_PROGRESS = 54;
	public static final int RCC_INVALID_JOB_ID = 55;
	public static final int RCC_INVALID_SCRIPT_ID = 56;
	public static final int RCC_INVALID_SCRIPT_NAME = 57;
	public static final int RCC_UNKNOWN_MAP_NAME = 58;
	public static final int RCC_INVALID_MAP_ID = 59;
	public static final int RCC_ACCOUNT_DISABLED = 60;
	public static final int RCC_NO_GRACE_LOGINS = 61;
	public static final int RCC_CONNECTION_BROKEN = 62;
	public static final int RCC_INVALID_CONFIG_ID = 63;
	public static final int RCC_DB_CONNECTION_LOST = 64;
	public static final int RCC_ALARM_OPEN_IN_HELPDESK = 65;
	public static final int RCC_ALARM_NOT_OUTSTANDING = 66;
	public static final int RCC_NOT_PUSH_DCI = 67;
	public static final int RCC_NXMP_PARSE_ERROR = 68;
	public static final int RCC_NXMP_VALIDATION_ERROR = 69;
	public static final int RCC_INVALID_GRAPH_ID = 70;
	public static final int RCC_LOCAL_CRYPTO_ERROR = 71;
	public static final int RCC_UNSUPPORTED_AUTH_TYPE = 72;
	public static final int RCC_BAD_CERTIFICATE = 73;
	public static final int RCC_INVALID_CERT_ID = 74;
	public static final int RCC_SNMP_FAILURE = 75;
	public static final int RCC_NO_L2_TOPOLOGY_SUPPORT = 76;
	public static final int RCC_INVALID_SITUATION_ID = 77;
	public static final int RCC_INSTANCE_NOT_FOUND = 78;
	public static final int RCC_INVALID_EVENT_ID = 79;
	public static final int RCC_AGENT_ERROR = 80;
	public static final int RCC_UNKNOWN_VARIABLE = 81;
	public static final int RCC_RESOURCE_NOT_AVAILABLE = 82;
	public static final int RCC_JOB_CANCEL_FAILED = 83;

	// User object fields
	public static final int USER_MODIFY_LOGIN_NAME = 0x00000001;
	public static final int USER_MODIFY_DESCRIPTION = 0x00000002;
	public static final int USER_MODIFY_FULL_NAME = 0x00000004;
	public static final int USER_MODIFY_FLAGS = 0x00000008;
	public static final int USER_MODIFY_ACCESS_RIGHTS = 0x00000010;
	public static final int USER_MODIFY_MEMBERS = 0x00000020;
	public static final int USER_MODIFY_CERT_MAPPING = 0x00000040;
	public static final int USER_MODIFY_AUTH_METHOD = 0x00000080;

	// Private constants
	private static final int CLIENT_CHALLENGE_SIZE = 256;
	private static final int MAX_DCI_DATA_ROWS = 200000;
	private static final int MAX_DCI_STRING_VALUE_LENGTH = 256;
	private static final int RECEIVED_FILE_TTL = 300000; // 300 secunds

	// Internal synchronization objects
	private final Semaphore syncObjects = new Semaphore(1);
	private final Semaphore syncUserDB = new Semaphore(1);

	// Connection-related attributes
	private String connAddress;
	private int connPort;
	private String connLoginName;
	private String connPassword;
	private boolean connUseEncryption;
	private String connClientInfo = "nxjclient/" + NetXMSConst.VERSION;

	// Information about logged in user
	private int userId;
	private int userSystemRights;

	// Internal communication data
	private Socket connSocket = null;
	private NXCPMsgWaitQueue msgWaitQueue = null;
	private ReceiverThread recvThread = null;
	private HousekeeperThread housekeeperThread = null;
	private long requestId = 0;
	private boolean isConnected = false;

	// Communication parameters
	private int recvBufferSize = 4194304; // Default is 4MB
	private int commandTimeout = 30000; // Default is 30 sec

	// Notification listeners
	private HashSet<NXCListener> listeners = new HashSet<NXCListener>(0);

	// Received files
	private Map<Long, NXCReceivedFile> receivedFiles = new HashMap<Long, NXCReceivedFile>();

	// Server information
	private String serverVersion = "(unknown)";
	private byte[] serverId = new byte[8];
	private String serverTimeZone;
	private byte[] serverChallenge = new byte[CLIENT_CHALLENGE_SIZE];

	// Objects
	private Map<Long, NXCObject> objectList = new HashMap<Long, NXCObject>();

	// Users
	private Map<Long, NXCUserDBObject> userDB = new HashMap<Long, NXCUserDBObject>();

	/**
	 * Create object from message
	 * 
	 * @param msg
	 *           Source NXCP message
	 * @return NetXMS object
	 */
	private NXCObject createObjectFromMessage(NXCPMessage msg)
	{
		final int objectClass = msg.getVariableAsInteger(NXCPCodes.VID_OBJECT_CLASS);
		NXCObject object;

		switch(objectClass)
		{
			case NXCObject.OBJECT_INTERFACE:
				object = new NXCInterface(msg, this);
				break;
			case NXCObject.OBJECT_SUBNET:
				object = new NXCSubnet(msg, this);
				break;
			case NXCObject.OBJECT_CONTAINER:
				object = new NXCContainer(msg, this);
				break;
			case NXCObject.OBJECT_NODE:
				object = new NXCNode(msg, this);
				break;
			case NXCObject.OBJECT_TEMPLATE:
				object = new NXCTemplate(msg, this);
				break;
			case NXCObject.OBJECT_NETWORK:
				object = new NXCEntireNetwork(msg, this);
				break;
			case NXCObject.OBJECT_SERVICEROOT:
				object = new NXCServiceRoot(msg, this);
				break;
			case NXCObject.OBJECT_POLICYROOT:
				object = new NXCPolicyRoot(msg, this);
				break;
			case NXCObject.OBJECT_POLICYGROUP:
				object = new NXCPolicyGroup(msg, this);
				break;
			case NXCObject.OBJECT_AGENTPOLICY:
				object = new NXCAgentPolicy(msg, this);
				break;
			case NXCObject.OBJECT_AGENTPOLICY_CONFIG:
				object = new NXCAgentPolicyConfig(msg, this);
				break;
			default:
				object = new NXCObject(msg, this);
				break;
		}

		return object;
	}

	/**
	 * Receiver thread for NXCSession
	 * 
	 * @author victor
	 */
	private class ReceiverThread extends Thread
	{
		ReceiverThread()
		{
			setDaemon(true);
			start();
		}

		@Override
		public void run()
		{
			final NXCPMessageReceiver receiver = new NXCPMessageReceiver(recvBufferSize);
			InputStream in;

			try
			{
				in = connSocket.getInputStream();
			}
			catch(IOException e)
			{
				return; // Stop receiver thread if input stream cannot be obtained
			}

			while(connSocket.isConnected())
			{
				try
				{
					final NXCPMessage msg = receiver.receiveMessage(in);
					switch(msg.getMessageCode())
					{
						case NXCPCodes.CMD_OBJECT:
						case NXCPCodes.CMD_OBJECT_UPDATE:
							final NXCObject obj = createObjectFromMessage(msg);
							synchronized(objectList)
							{
								if (obj.isDeleted())
									objectList.remove(obj.getObjectId());
								else
									objectList.put(obj.getObjectId(), obj);
							}
							if (msg.getMessageCode() == NXCPCodes.CMD_OBJECT_UPDATE)
								sendNotification(new NXCNotification(NXCNotification.OBJECT_CHANGED, obj));
							break;
						case NXCPCodes.CMD_OBJECT_LIST_END:
							completeSync(syncObjects);
							break;
						case NXCPCodes.CMD_USER_DATA:
							final NXCUser user = new NXCUser(msg);
							synchronized(userDB)
							{
								if (user.isDeleted())
									userDB.remove(user.getId());
								else
									userDB.put(user.getId(), user);
							}
							break;
						case NXCPCodes.CMD_GROUP_DATA:
							final NXCUserGroup group = new NXCUserGroup(msg);
							synchronized(userDB)
							{
								if (group.isDeleted())
									userDB.remove(group.getId());
								else
									userDB.put(group.getId(), group);
							}
							break;
						case NXCPCodes.CMD_USER_DB_EOF:
							completeSync(syncUserDB);
							break;
						case NXCPCodes.CMD_USER_DB_UPDATE:
							processUserDBUpdate(msg);
							break;
						case NXCPCodes.CMD_ALARM_UPDATE:
							sendNotification(new NXCNotification(msg.getVariableAsInteger(NXCPCodes.VID_NOTIFICATION_CODE)
									+ NXCNotification.NOTIFY_BASE, new NXCAlarm(msg)));
							break;
						case NXCPCodes.CMD_JOB_CHANGE_NOTIFICATION:
							sendNotification(new NXCNotification(NXCNotification.JOB_CHANGE, new NXCServerJob(msg)));
							break;
						case NXCPCodes.CMD_FILE_DATA:
							processFileData(msg);
							break;
						default:
							if (msg.getMessageCode() >= 0x1000)
							{
								// Custom message
								sendNotification(new NXCNotification(NXCNotification.CUSTOM_MESSAGE, msg));
							}
							msgWaitQueue.putMessage(msg);
							break;
					}
				}
				catch(IOException e)
				{
					break; // Stop on read errors
				}
				catch(NXCPException e)
				{
				}
			}
		}

		/**
		 * Process file data
		 * 
		 * @param msg
		 */
		private void processFileData(NXCPMessage msg)
		{
			long id = msg.getMessageId();
			NXCReceivedFile file;
			synchronized(receivedFiles)
			{
				file = receivedFiles.get(id);
				if (file == null)
				{
					file = new NXCReceivedFile(id);
					receivedFiles.put(id, file);
				}
			}
			file.writeData(msg.getBinaryData());
			if (msg.isEndOfFileSet())
			{
				file.close();
				synchronized(receivedFiles)
				{
					receivedFiles.notifyAll();
				}
			}
		}

		/**
		 * Process updates in user database
		 * 
		 * @param msg
		 *           Notification message
		 */
		private void processUserDBUpdate(final NXCPMessage msg)
		{
			final int code = msg.getVariableAsInteger(NXCPCodes.VID_UPDATE_TYPE);
			final long id = msg.getVariableAsInt64(NXCPCodes.VID_USER_ID);

			NXCUserDBObject object = null;
			switch(code)
			{
				case NXCNotification.USER_DB_OBJECT_CREATED:
				case NXCNotification.USER_DB_OBJECT_MODIFIED:
					object = ((id & 0x80000000) != 0) ? new NXCUserGroup(msg) : new NXCUser(msg);
					synchronized(userDB)
					{
						userDB.put(id, object);
					}
					break;
				case NXCNotification.USER_DB_OBJECT_DELETED:
					synchronized(userDB)
					{
						object = userDB.get(id);
						if (object != null)
						{
							userDB.remove(id);
						}
					}
					break;
			}

			// Send notification if changed object was found in local database copy
			// or added to it and notification code was known
			if (object != null)
				sendNotification(new NXCNotification(NXCNotification.USER_DB_CHANGED, code, object));
		}
	}

	/**
	 * Housekeeper thread - cleans received files, etc.
	 * 
	 * @author Victor
	 * 
	 */
	private class HousekeeperThread extends Thread
	{
		private boolean stopFlag = false;

		HousekeeperThread()
		{
			setDaemon(true);
			start();
		}

		/* (non-Javadoc)
		 * @see java.lang.Thread#run()
		 */
		@Override
		public void run()
		{
			while(!stopFlag)
			{
				try
				{
					sleep(1000);
				}
				catch(InterruptedException e)
				{
				}

				// Check for old entries in received files
				synchronized(receivedFiles)
				{
					long currTime = System.currentTimeMillis();
					Iterator<NXCReceivedFile> it = receivedFiles.values().iterator();
					while(it.hasNext())
					{
						NXCReceivedFile file = it.next();
						if (file.getTimestamp() + RECEIVED_FILE_TTL < currTime)
							it.remove();
					}
				}
			}
		}

		/**
		 * @param stopFlag
		 *           the stopFlag to set
		 */
		public void setStopFlag(boolean stopFlag)
		{
			this.stopFlag = stopFlag;
		}
	}

	//
	// Constructors
	//

	/**
	 * @param connAddress
	 * @param connLoginName
	 * @param connPassword
	 */
	public NXCSession(String connAddress, String connLoginName, String connPassword)
	{
		this.connAddress = connAddress;
		this.connPort = DEFAULT_CONN_PORT;
		this.connLoginName = connLoginName;
		this.connPassword = connPassword;
		this.connUseEncryption = false;
	}

	/**
	 * @param connAddress
	 * @param connPort
	 * @param connLoginName
	 * @param connPassword
	 */
	public NXCSession(String connAddress, int connPort, String connLoginName, String connPassword)
	{
		this.connAddress = connAddress;
		this.connPort = connPort;
		this.connLoginName = connLoginName;
		this.connPassword = connPassword;
		this.connUseEncryption = false;
	}

	/**
	 * @param connAddress
	 * @param connPort
	 * @param connLoginName
	 * @param connPassword
	 * @param connUseEncryption
	 */
	public NXCSession(String connAddress, int connPort, String connLoginName, String connPassword,
			boolean connUseEncryption)
	{
		this.connAddress = connAddress;
		this.connPort = connPort;
		this.connLoginName = connLoginName;
		this.connPassword = connPassword;
		this.connUseEncryption = connUseEncryption;
	}

	//
	// Finalize
	//

	@Override
	protected void finalize()
	{
		disconnect();
	}

	//
	// Wait for synchronization
	//

	private void waitForSync(final Semaphore syncObject, final int timeout) throws NXCException
	{
		if (timeout == 0)
		{
			syncObject.acquireUninterruptibly();
		}
		else
		{
			long actualTimeout = timeout;
			boolean success = false;

			while(actualTimeout > 0)
			{
				long startTime = System.currentTimeMillis();
				try
				{
					if (syncObjects.tryAcquire(actualTimeout, TimeUnit.MILLISECONDS))
					{
						success = true;
						syncObjects.release();
						break;
					}
				}
				catch(InterruptedException e)
				{
				}
				actualTimeout -= System.currentTimeMillis() - startTime;
			}
			if (!success)
				throw new NXCException(RCC_TIMEOUT);
		}
	}

	/**
	 * Report synchronization completion
	 * 
	 * @param syncObject
	 *           Synchronization object
	 */
	private void completeSync(final Semaphore syncObject)
	{
		syncObject.release();
	}

	/**
	 * Add notification listener
	 * 
	 * @param lst
	 *           Listener to add
	 */
	public void addListener(NXCListener lst)
	{
		listeners.add(lst);
	}

	/**
	 * Remove notification listener
	 * 
	 * @param lst
	 *           Listener to remove
	 */
	public void removeListener(NXCListener lst)
	{
		listeners.remove(lst);
	}

	/**
	 * Call notification handlers on all registered listeners
	 * 
	 * @param n
	 *           Notification object
	 */
	protected synchronized void sendNotification(NXCNotification n)
	{
		Iterator<NXCListener> it = listeners.iterator();
		while(it.hasNext())
		{
			it.next().notificationHandler(n);
		}
	}

	/**
	 * Send message to server
	 * 
	 * @param msg
	 *           Message to sent
	 * @throws IOException
	 *            if case of socket communication failure
	 */
	public synchronized void sendMessage(final NXCPMessage msg) throws IOException
	{
		connSocket.getOutputStream().write(msg.createNXCPMessage());
	}

	/**
	 * Wait for message with specific code and id.
	 * 
	 * @param code
	 *           Message code
	 * @param id
	 *           Message id
	 * @param timeout
	 *           Wait timeout in milliseconds
	 * @return Message object
	 * @throws NXCException
	 *            if message was not arrived within timeout interval
	 */
	public NXCPMessage waitForMessage(final int code, final long id, final int timeout) throws NXCException
	{
		final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id, timeout);
		if (msg == null)
			throw new NXCException(RCC_TIMEOUT);
		return msg;
	}

	/**
	 * Wait for message with specific code and id.
	 * 
	 * @param code
	 *           Message code
	 * @param id
	 *           Message id
	 * @return Message object
	 * @throws NXCException
	 *            if message was not arrived within timeout interval
	 */
	public NXCPMessage waitForMessage(final int code, final long id) throws NXCException
	{
		final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id);
		if (msg == null)
			throw new NXCException(RCC_TIMEOUT);
		return msg;
	}

	/**
	 * Wait for CMD_REQUEST_COMPLETED message with given id
	 * 
	 * @param id
	 *           Message id
	 * @return received message
	 * @throws NXCException
	 *            if message was not arrived within timeout interval or contains RCC other than RCC_SUCCESS
	 */
	public NXCPMessage waitForRCC(final long id) throws NXCException
	{
		final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id);
		final int rcc = msg.getVariableAsInteger(NXCPCodes.VID_RCC);
		if (rcc != RCC_SUCCESS)
			throw new NXCException(rcc);
		return msg;
	}

	/**
	 * Create new NXCP message with unique id
	 * 
	 * @param code
	 *           Message code
	 * @return New message object
	 */
	public final synchronized NXCPMessage newMessage(int code)
	{
		return new NXCPMessage(code, requestId++);
	}

	/**
	 * Wait for specific file to arrive
	 * 
	 * @param id
	 *           Message ID
	 * @param timeout
	 *           Wait timeout in milliseconds
	 * @return Received file or null in case of failure
	 */
	public File waitForFile(final long id, final int timeout)
	{
		int timeRemaining = timeout;
		File file = null;

		while(timeRemaining > 0)
		{
			synchronized(receivedFiles)
			{
				NXCReceivedFile rf = receivedFiles.get(id);
				if (rf != null)
				{
					if (rf.getStatus() != NXCReceivedFile.OPEN)
					{
						if (rf.getStatus() == NXCReceivedFile.RECEIVED)
							file = rf.getFile();
						break;
					}
				}

				long startTime = System.currentTimeMillis();
				try
				{
					receivedFiles.wait(timeRemaining);
				}
				catch(InterruptedException e)
				{
				}
				timeRemaining -= System.currentTimeMillis() - startTime;
			}
		}
		return file;
	}

	/**
	 * Connect to server using previously set credentials.
	 * 
	 * @throws IOException
	 *            to indicate socket I/O error
	 * @throws UnknownHostException
	 *            if given host name cannot be resolved
	 * @throws NXCException
	 *            if NetXMS server returns an error, protocol negotiation with the server was failed, or operation was
	 *            timed out
	 */

	public void connect() throws IOException, UnknownHostException, NXCException
	{
		try
		{
			connSocket = new Socket(connAddress, connPort);
			msgWaitQueue = new NXCPMsgWaitQueue(commandTimeout);
			recvThread = new ReceiverThread();
			housekeeperThread = new HousekeeperThread();

			// get server information
			NXCPMessage request = newMessage(NXCPCodes.CMD_GET_SERVER_INFO);
			sendMessage(request);
			NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());

			if (response.getVariableAsInteger(NXCPCodes.VID_PROTOCOL_VERSION) != CLIENT_PROTOCOL_VERSION)
				throw new NXCException(RCC_BAD_PROTOCOL);

			serverVersion = response.getVariableAsString(NXCPCodes.VID_SERVER_VERSION);
			serverId = response.getVariableAsBinary(NXCPCodes.VID_SERVER_ID);
			serverTimeZone = response.getVariableAsString(NXCPCodes.VID_TIMEZONE);
			serverChallenge = response.getVariableAsBinary(NXCPCodes.VID_CHALLENGE);

			// Setup encryption if required
			if (connUseEncryption)
			{
				/* TODO: implement encryption setup */
			}

			// Login to server
			request = newMessage(NXCPCodes.CMD_LOGIN);
			request.setVariable(NXCPCodes.VID_LOGIN_NAME, connLoginName);
			request.setVariable(NXCPCodes.VID_PASSWORD, connPassword);
			request.setVariableInt16(NXCPCodes.VID_AUTH_TYPE, AUTH_TYPE_PASSWORD);
			request.setVariable(NXCPCodes.VID_LIBNXCL_VERSION, NetXMSConst.VERSION);
			request.setVariable(NXCPCodes.VID_CLIENT_INFO, connClientInfo);
			request.setVariable(NXCPCodes.VID_OS_INFO, System.getProperty("os.name") + " "
					+ System.getProperty("os.version"));
			sendMessage(request);
			response = waitForMessage(NXCPCodes.CMD_LOGIN_RESP, request.getMessageId());
			int rcc = response.getVariableAsInteger(NXCPCodes.VID_RCC);
			if (rcc != RCC_SUCCESS)
				throw new NXCException(rcc);
			userId = response.getVariableAsInteger(NXCPCodes.VID_USER_ID);
			userSystemRights = response.getVariableAsInteger(NXCPCodes.VID_USER_SYS_RIGHTS);

			isConnected = true;
		}
		finally
		{
			if (!isConnected)
				disconnect();
		}
	}

	/**
	 * Disconnect from server.
	 * 
	 */

	public void disconnect()
	{
		if (connSocket != null)
		{
			try
			{
				connSocket.close();
			}
			catch(IOException e)
			{
			}
		}

		if (recvThread != null)
		{
			while(recvThread.isAlive())
			{
				try
				{
					recvThread.join();
				}
				catch(InterruptedException e)
				{
				}
			}
			recvThread = null;
		}

		if (housekeeperThread != null)
		{
			housekeeperThread.setStopFlag(true);
			while(housekeeperThread.isAlive())
			{
				try
				{
					housekeeperThread.join();
				}
				catch(InterruptedException e)
				{
				}
			}
			housekeeperThread = null;
		}

		connSocket = null;

		if (msgWaitQueue != null)
		{
			msgWaitQueue.shutdown();
			msgWaitQueue = null;
		}

		isConnected = false;
	}

	/**
	 * Get receiver buffer size.
	 * 
	 * @return Current receiver buffer size in bytes.
	 */
	public int getRecvBufferSize()
	{
		return recvBufferSize;
	}

	/**
	 * Set receiver buffer size. This method should be called before connect(). It will not have any effect after
	 * connect().
	 * 
	 * @param recvBufferSize
	 *           Size of receiver buffer in bytes.
	 */
	public void setRecvBufferSize(int recvBufferSize)
	{
		this.recvBufferSize = recvBufferSize;
	}

	/**
	 * Get NetXMS server version.
	 * 
	 * @return Server version
	 */
	public String getServerVersion()
	{
		return serverVersion;
	}

	/**
	 * Get NetXMS server UID.
	 * 
	 * @return Server UID
	 */
	public byte[] getServerId()
	{
		return serverId;
	}

	/**
	 * @return the serverTimeZone
	 */
	public String getServerTimeZone()
	{
		return serverTimeZone;
	}

	/**
	 * @return the serverChallenge
	 */
	public byte[] getServerChallenge()
	{
		return serverChallenge;
	}

	/**
	 * @return the connClientInfo
	 */
	public String getConnClientInfo()
	{
		return connClientInfo;
	}

	/**
	 * @param connClientInfo
	 *           the connClientInfo to set
	 */
	public void setConnClientInfo(final String connClientInfo)
	{
		this.connClientInfo = connClientInfo;
	}

	/**
	 * Set command execution timeout.
	 * 
	 * @param commandTimeout
	 *           New command timeout
	 */
	public void setCommandTimeout(final int commandTimeout)
	{
		this.commandTimeout = commandTimeout;
	}

	/**
	 * Get identifier of logged in user.
	 * 
	 * @return Identifier of logged in user
	 */
	public int getUserId()
	{
		return userId;
	}

	/**
	 * Get system-wide rights of currently logged in user.
	 * 
	 * @return System-wide rights of currently logged in user
	 */

	public int getUserSystemRights()
	{
		return userSystemRights;
	}

	/**
	 * Synchronizes NetXMS objects between server and client. After successful sync, subscribe client to object change
	 * notifications.
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */

	public synchronized void syncObjects() throws IOException, NXCException
	{
		syncObjects.acquireUninterruptibly();
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECTS);
		msg.setVariableInt16(NXCPCodes.VID_SYNC_COMMENTS, 1);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
		waitForSync(syncObjects, commandTimeout * 10);
		subscribe(CHANNEL_OBJECTS);
	}

	/**
	 * Find NetXMS object by it's identifier.
	 * 
	 * @param id
	 *           Object identifier
	 * @return Object with given ID or null if object cannot be found
	 */
	public NXCObject findObjectById(final long id)
	{
		NXCObject obj;

		synchronized(objectList)
		{
			obj = objectList.get(id);
		}
		return obj;
	}

	/**
	 * Find multiple NetXMS objects by identifiers
	 * 
	 * @param idList
	 *           array of object identifiers
	 * @return array of found objects
	 */
	public NXCObject[] findMultipleObjects(final long[] idList)
	{
		ArrayList<NXCObject> result = new ArrayList<NXCObject>(idList.length);

		synchronized(objectList)
		{
			for(int i = 0; i < idList.length; i++)
			{
				final NXCObject object = objectList.get(idList[i]);
				if (object != null)
					result.add(object);
			}
		}

		return result.toArray(new NXCObject[result.size()]);
	}

	/**
	 * Get list of top-level objects.
	 * 
	 * @return List of all top level objects (either without parents or with inaccessible parents)
	 */
	public NXCObject[] getTopLevelObjects()
	{
		HashSet<NXCObject> list = new HashSet<NXCObject>();
		list.add(findObjectById(1));
		list.add(findObjectById(2));
		list.add(findObjectById(3));
		list.add(findObjectById(5));
		return list.toArray(new NXCObject[list.size()]);
	}

	/**
	 * Get list of all objects
	 * 
	 * @return List of all objects
	 */
	public NXCObject[] getAllObjects()
	{
		NXCObject[] list;

		synchronized(objectList)
		{
			list = objectList.values().toArray(new NXCObject[objectList.size()]);
		}
		return list;
	}

	/**
	 * Get alarm list.
	 * 
	 * @param getTerminated
	 *           if set to true, all alarms will be retrieved from database, otherwise only active alarms
	 * @return Hash map containing alarms
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public HashMap<Long, NXCAlarm> getAlarms(final boolean getTerminated) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALL_ALARMS);
		final long rqId = msg.getMessageId();

		msg.setVariableInt16(NXCPCodes.VID_IS_ACK, getTerminated ? 1 : 0);
		sendMessage(msg);

		final HashMap<Long, NXCAlarm> alarmList = new HashMap<Long, NXCAlarm>(0);
		while(true)
		{
			msg = waitForMessage(NXCPCodes.CMD_ALARM_DATA, rqId);
			long alarmId = msg.getVariableAsInteger(NXCPCodes.VID_ALARM_ID);
			if (alarmId == 0)
				break; // ALARM_ID == 0 indicates end of list
			alarmList.put(alarmId, new NXCAlarm(msg));
		}

		return alarmList;
	}

	/**
	 * Acknowledge alarm.
	 * 
	 * @param alarmId
	 *           Identifier of alarm to be acknowledged.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void acknowledgeAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_ACK_ALARM);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Terminate alarm.
	 * 
	 * @param alarmId
	 *           Identifier of alarm to be terminated.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void terminateAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_TERMINATE_ALARM);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Delete alarm.
	 * 
	 * @param alarmId
	 *           Identifier of alarm to be deleted.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void deleteAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ALARM);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Set alarm's helpdesk state to "Open".
	 * 
	 * @param alarmId
	 *           Identifier of alarm to be changed.
	 * @param reference
	 *           Helpdesk reference string.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void openAlarm(final long alarmId, final String reference) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ALARM_HD_STATE);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
		msg.setVariableInt16(NXCPCodes.VID_HELPDESK_STATE, NXCAlarm.HELPDESK_STATE_OPEN);
		msg.setVariable(NXCPCodes.VID_HELPDESK_REF, reference);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Set alarm's helpdesk state to "Closed".
	 * 
	 * @param alarmId
	 *           Identifier of alarm to be changed.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void closeAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ALARM_HD_STATE);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int) alarmId);
		msg.setVariableInt16(NXCPCodes.VID_HELPDESK_STATE, NXCAlarm.HELPDESK_STATE_CLOSED);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Get server configuration variables.
	 * 
	 * @return Hash map containing server configuration variables
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public HashMap<String, NXCServerVariable> getServerVariables() throws IOException, NXCException
	{
		NXCPMessage request = newMessage(NXCPCodes.CMD_GET_CONFIG_VARLIST);
		sendMessage(request);

		final NXCPMessage response = waitForRCC(request.getMessageId());

		long id;
		int i, count = response.getVariableAsInteger(NXCPCodes.VID_NUM_VARIABLES);
		final HashMap<String, NXCServerVariable> varList = new HashMap<String, NXCServerVariable>(count);
		for(i = 0, id = NXCPCodes.VID_VARLIST_BASE; i < count; i++, id += 3)
		{
			String name = response.getVariableAsString(id);
			varList.put(name, new NXCServerVariable(name, response.getVariableAsString(id + 1), response
					.getVariableAsBoolean(id + 2)));
		}

		return varList;
	}

	/**
	 * Set server configuration variable.
	 * 
	 * @param name
	 *           variable's name
	 * @param value
	 *           new variable's value
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void setServerVariable(final String name, final String value) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_CONFIG_VARIABLE);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_VALUE, value);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Delete server configuration variable.
	 * 
	 * @param name
	 *           variable's name
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void deleteServerVariable(final String name) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_CONFIG_VARIABLE);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Subscribe to notification channel(s)
	 * 
	 * @param channels
	 *           Notification channels to subscribe to. Multiple channels can be specified by combining them with OR
	 *           operation.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void subscribe(int channels) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, channels);
		msg.setVariableInt16(NXCPCodes.VID_OPERATION, 1);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Unsubscribe from notification channel(s)
	 * 
	 * @param channels
	 *           Notification channels to unsubscribe from. Multiple channels can be specified by combining them with OR
	 *           operation.
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void unsubscribe(int channels) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, channels);
		msg.setVariableInt16(NXCPCodes.VID_OPERATION, 0);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Synchronize user database
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void syncUserDatabase() throws IOException, NXCException
	{
		syncUserDB.acquireUninterruptibly();
		NXCPMessage msg = newMessage(NXCPCodes.CMD_LOAD_USER_DB);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
		waitForSync(syncUserDB, commandTimeout * 10);
	}

	/**
	 * Find user by ID
	 * 
	 * @return User object with given ID or null if such user does not exist
	 */
	public NXCUserDBObject findUserDBObjectById(final long id)
	{
		NXCUserDBObject object;

		synchronized(userDB)
		{
			object = userDB.get(id);
		}
		return object;
	}

	/**
	 * Get list of all user database objects
	 * 
	 * @return List of all user database objects
	 */
	public NXCUserDBObject[] getUserDatabaseObjects()
	{
		NXCUserDBObject[] list;

		synchronized(userDB)
		{
			Collection<NXCUserDBObject> values = userDB.values();
			list = values.toArray(new NXCUserDBObject[values.size()]);
		}
		return list;
	}

	/**
	 * Create user or group on server
	 * 
	 * @param name
	 *           Login name for new user
	 * @return ID assigned to newly created user
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	private long createUserDBObject(final String name, final boolean isGroup) throws IOException, NXCException
	{
		final NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_USER);
		msg.setVariable(NXCPCodes.VID_USER_NAME, name);
		msg.setVariableInt16(NXCPCodes.VID_IS_GROUP, isGroup ? 1 : 0);
		sendMessage(msg);

		final NXCPMessage response = waitForRCC(msg.getMessageId());
		return response.getVariableAsInt64(NXCPCodes.VID_USER_ID);
	}

	/**
	 * Create user on server
	 * 
	 * @param name
	 *           Login name for new user
	 * @return ID assigned to newly created user
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public long createUser(final String name) throws IOException, NXCException
	{
		return createUserDBObject(name, false);
	}

	/**
	 * Create user group on server
	 * 
	 * @param name
	 *           Name for new user group
	 * @return ID assigned to newly created user group
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public long createUserGroup(final String name) throws IOException, NXCException
	{
		return createUserDBObject(name, true);
	}

	/**
	 * Delete user or group on server
	 * 
	 * @param id
	 *           User or group ID
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void deleteUserDBObject(final long id) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_USER);
		msg.setVariableInt32(NXCPCodes.VID_USER_ID, (int) id);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Set password for user
	 * 
	 * @param id
	 *           User ID
	 * @param password
	 *           New password
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void setUserPassword(final long id, final String password) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_PASSWORD);
		msg.setVariableInt32(NXCPCodes.VID_USER_ID, (int) id);

		MessageDigest md;
		try
		{
			md = MessageDigest.getInstance("SHA-1");
		}
		catch(NoSuchAlgorithmException e)
		{
			throw new NXCException(RCC_INTERNAL_ERROR);
		}
		byte[] digest = md.digest(password.getBytes());
		msg.setVariable(NXCPCodes.VID_PASSWORD, digest);

		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Modify user database object
	 * 
	 * @param user
	 *           User data
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void modifyUserDBObject(final NXCUserDBObject object, final int fields) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_UPDATE_USER);
		msg.setVariableInt32(NXCPCodes.VID_FIELDS, fields);
		object.fillMessage(msg);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Modify user database object
	 * 
	 * @param user
	 *           User data
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void modifyUserDBObject(final NXCUserDBObject object) throws IOException, NXCException
	{
		modifyUserDBObject(object, 0x7FFFFFFF);
	}

	/**
	 * Lock user database
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void lockUserDatabase() throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_LOCK_USER_DB);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Unlock user database
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void unlockUserDatabase() throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_UNLOCK_USER_DB);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Get last DCI values for given node
	 * 
	 * @param nodeId
	 *           ID of the node to get DCI values for
	 * @return List of DCI values
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public NXCDCIValue[] getLastValues(final long nodeId) throws IOException, NXCException
	{
		final NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_LAST_VALUES);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
		sendMessage(msg);

		final NXCPMessage response = waitForRCC(msg.getMessageId());

		int count = response.getVariableAsInteger(NXCPCodes.VID_NUM_ITEMS);
		NXCDCIValue[] list = new NXCDCIValue[count];
		long base = NXCPCodes.VID_DCI_VALUES_BASE;
		for(int i = 0; i < count; i++, base += 10)
			list[i] = new NXCDCIValue(nodeId, response, base);

		return list;
	}

	/**
	 * Parse data from raw message CMD_DCI_DATA
	 * 
	 * @param input
	 *           Raw data
	 * @param data
	 *           Data object to add rows to
	 * @return number of received data rows
	 */
	private int parseDataRows(final byte[] input, NXCDCIData data)
	{
		//noinspection IOResourceOpenedButNotSafelyClosed
		final NXCPDataInputStream inputStream = new NXCPDataInputStream(input);
		int rows = 0;

		try
		{
			inputStream.skipBytes(4); // DCI ID
			rows = inputStream.readInt();
			final int dataType = inputStream.readInt();

			for(int i = 0; i < rows; i++)
			{
				long timestamp = inputStream.readUnsignedInt() * 1000; // convert to milliseconds

				switch(dataType)
				{
					case NXCDCI.DT_INT:
						data.addDataRow(new NXCDCIDataRow(new Date(timestamp), new Long(inputStream.readInt())));
						break;
					case NXCDCI.DT_UINT:
						data.addDataRow(new NXCDCIDataRow(new Date(timestamp), new Long(inputStream.readUnsignedInt())));
						break;
					case NXCDCI.DT_INT64:
					case NXCDCI.DT_UINT64:
						data.addDataRow(new NXCDCIDataRow(new Date(timestamp), new Long(inputStream.readLong())));
						break;
					case NXCDCI.DT_FLOAT:
						data.addDataRow(new NXCDCIDataRow(new Date(timestamp), new Double(inputStream.readDouble())));
						break;
					case NXCDCI.DT_STRING:
						StringBuilder sb = new StringBuilder(256);
						int count;
						for(count = MAX_DCI_STRING_VALUE_LENGTH; count > 0; count--)
						{
							char ch = inputStream.readChar();
							if (ch == 0)
							{
								count--;
								break;
							}
							sb.append(ch);
						}
						inputStream.skipBytes(count);
						data.addDataRow(new NXCDCIDataRow(new Date(timestamp), sb.toString()));
						break;
				}
			}
		}
		catch(IOException e)
		{
		}

		return rows;
	}

	/**
	 * Get collected DCI data from server. Please note that you should specify either row count limit or time from/to
	 * limit.
	 * 
	 * @param nodeId
	 *           Node ID
	 * @param dciId
	 *           DCI ID
	 * @param from
	 *           Start of time range or null for no limit
	 * @param to
	 *           End of time range or null for no limit
	 * @param maxRows
	 *           Maximum number of rows to retrieve or 0 for no limit
	 * @return DCI data set
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public NXCDCIData getCollectedData(final long nodeId, final long dciId, Date from, Date to, int maxRows)
			throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_DCI_DATA);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
		msg.setVariableInt32(NXCPCodes.VID_DCI_ID, (int) dciId);

		NXCDCIData data = new NXCDCIData(nodeId, dciId);

		int rowsReceived, rowsRemaining = maxRows;
		int timeFrom = (from != null) ? (int) (from.getTime() / 1000) : 0;
		int timeTo = (to != null) ? (int) (to.getTime() / 1000) : 0;

		do
		{
			msg.setMessageId(requestId++);
			msg.setVariableInt32(NXCPCodes.VID_MAX_ROWS, maxRows);
			msg.setVariableInt32(NXCPCodes.VID_TIME_FROM, timeFrom);
			msg.setVariableInt32(NXCPCodes.VID_TIME_TO, timeTo);
			sendMessage(msg);

			waitForRCC(msg.getMessageId());

			NXCPMessage response = waitForMessage(NXCPCodes.CMD_DCI_DATA, msg.getMessageId());
			if (!response.isRawMessage())
				throw new NXCException(RCC_INTERNAL_ERROR);

			rowsReceived = parseDataRows(response.getBinaryData(), data);
			if (((rowsRemaining == 0) || (rowsRemaining > MAX_DCI_DATA_ROWS)) && (rowsReceived == MAX_DCI_DATA_ROWS))
			{
				// adjust boundaries for next request
				if (rowsRemaining > 0)
					rowsRemaining -= rowsReceived;

				// Rows goes in newest to oldest order, so if we need to
				// retrieve additional data, we should update timeTo limit
				if (to != null)
				{
					NXCDCIDataRow row = data.getLastValue();
					if (row != null)
					{
						// There should be only one value per second, so we set
						// last row's timestamp - 1 second as new boundary
						timeTo = (int) (row.getTimestamp().getTime() / 1000) - 1;
					}
				}
			}
		} while(rowsReceived == MAX_DCI_DATA_ROWS);

		return data;
	}

	/**
	 * Create object
	 * 
	 * @param data
	 *           Object creation data
	 * @return ID of new object
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public long createObject(final NXCObjectCreationData data) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_CREATE_OBJECT);

		// Common attributes
		msg.setVariableInt32(NXCPCodes.VID_PARENT_ID, (int) data.getParentId());
		msg.setVariableInt16(NXCPCodes.VID_OBJECT_CLASS, data.getObjectClass());
		msg.setVariable(NXCPCodes.VID_OBJECT_NAME, data.getName());
		if (data.getComments() != null)
			msg.setVariable(NXCPCodes.VID_COMMENTS, data.getComments());

		// Class-specific attributes
		switch(data.getObjectClass())
		{
			case NXCObject.OBJECT_NODE:
				msg.setVariable(NXCPCodes.VID_IP_ADDRESS, data.getIpAddress());
				msg.setVariable(NXCPCodes.VID_IP_NETMASK, data.getIpNetMask());
				msg.setVariableInt32(NXCPCodes.VID_CREATION_FLAGS, data.getCreationFlags());
				msg.setVariableInt32(NXCPCodes.VID_PROXY_NODE, (int) data.getAgentProxyId());
				msg.setVariableInt32(NXCPCodes.VID_SNMP_PROXY, (int) data.getSnmpProxyId());
				break;
		}

		sendMessage(msg);
		NXCPMessage response = waitForRCC(msg.getMessageId());
		return response.getVariableAsInt64(NXCPCodes.VID_OBJECT_ID);
	}

	/**
	 * Delete object
	 * 
	 * @param objectId
	 *           ID of an object which should be deleted
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void deleteObject(final long objectId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_OBJECT);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) objectId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Modify object (generic interface, in most cases wrapper functions should be used instead)
	 * 
	 * @param data
	 *           Object modification data
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void modifyObject(final NXCObjectModificationData data) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_MODIFY_OBJECT);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) data.getObjectId());

		long flags = data.getFlags();
		if (flags == 0)
			return; // Nothing to change

		// Object name
		if ((flags & NXCObjectModificationData.MODIFY_NAME) != 0)
			msg.setVariable(NXCPCodes.VID_OBJECT_NAME, data.getName());

		// Access control list
		if ((flags & NXCObjectModificationData.MODIFY_ACL) != 0)
		{
			final NXCAccessListElement[] acl = data.getACL();
			msg.setVariableInt32(NXCPCodes.VID_ACL_SIZE, acl.length);
			msg.setVariableInt16(NXCPCodes.VID_INHERIT_RIGHTS, data.isInheritAccessRights() ? 1 : 0);

			long id1 = NXCPCodes.VID_ACL_USER_BASE;
			long id2 = NXCPCodes.VID_ACL_RIGHTS_BASE;
			for(int i = 0; i < acl.length; i++)
			{
				msg.setVariableInt32(id1++, acl[i].getUserId());
				msg.setVariableInt32(id2++, acl[i].getAccessRights());
			}
		}

		// Custom attributes
		if ((flags & NXCObjectModificationData.MODIFY_CUSTOM_ATTRIBUTES) != 0)
		{
			Map<String, String> attrList = data.getCustomAttributes();
			Iterator<String> it = attrList.keySet().iterator();
			long id = NXCPCodes.VID_CUSTOM_ATTRIBUTES_BASE;
			int count = 0;
			while(it.hasNext())
			{
				String key = it.next();
				String value = attrList.get(key);
				msg.setVariable(id++, key);
				msg.setVariable(id++, value);
				count++;
			}
			msg.setVariableInt32(NXCPCodes.VID_NUM_CUSTOM_ATTRIBUTES, count);
		}

		// Auto apply
		if ((flags & NXCObjectModificationData.MODIFY_AUTO_APPLY) != 0)
		{
			msg.setVariableInt16(NXCPCodes.VID_AUTO_APPLY, data.isAutoApplyEnabled() ? 1 : 0);
			msg.setVariable(NXCPCodes.VID_APPLY_FILTER, data.getAutoApplyFilter());
		}

		// Auto bind
		if ((flags & NXCObjectModificationData.MODIFY_AUTO_BIND) != 0)
		{
			msg.setVariableInt16(NXCPCodes.VID_ENABLE_AUTO_BIND, data.isAutoBindEnabled() ? 1 : 0);
			msg.setVariable(NXCPCodes.VID_AUTO_BIND_FILTER, data.getAutoBindFilter());
		}

		// Description
		if ((flags & NXCObjectModificationData.MODIFY_DESCRIPTION) != 0)
		{
			msg.setVariable(NXCPCodes.VID_DESCRIPTION, data.getDescription());
		}

		// Version
		if ((flags & NXCObjectModificationData.MODIFY_VERSION) != 0)
		{
			msg.setVariableInt32(NXCPCodes.VID_VERSION, data.getVersion());
		}

		// Configuration file
		if ((flags & NXCObjectModificationData.MODIFY_POLICY_CONFIG) != 0)
		{
			msg.setVariable(NXCPCodes.VID_CONFIG_FILE_NAME, data.getConfigFileName());
			msg.setVariable(NXCPCodes.VID_CONFIG_FILE_DATA, data.getConfigFileContent());
		}

		// Agent port
		if ((flags & NXCObjectModificationData.MODIFY_AGENT_PORT) != 0)
		{
			msg.setVariableInt32(NXCPCodes.VID_AGENT_PORT, data.getAgentPort());
		}

		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Change object's name (wrapper for modifyObject())
	 * 
	 * @param objectId
	 *           ID of object to be changed
	 * @param name
	 *           New object's name
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void setObjectName(final long objectId, final String name) throws IOException, NXCException
	{
		NXCObjectModificationData data = new NXCObjectModificationData(objectId);
		data.setName(name);
		modifyObject(data);
	}

	/**
	 * Change object's custom attributes (wrapper for modifyObject())
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void setObjectCustomAttributes(final long objectId, final Map<String, String> attrList) throws IOException,
			NXCException
	{
		NXCObjectModificationData data = new NXCObjectModificationData(objectId);
		data.setCustomAttributes(attrList);
		modifyObject(data);
	}

	/**
	 * Change object's ACL (wrapper for modifyObject())
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void setObjectACL(final long objectId, final NXCAccessListElement[] acl, final boolean inheritAccessRights)
			throws IOException, NXCException
	{
		NXCObjectModificationData data = new NXCObjectModificationData(objectId);
		data.setACL(acl);
		data.setInheritAccessRights(inheritAccessRights);
		modifyObject(data);
	}

	/**
	 * Query layer 2 topology for node
	 * 
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public NXCMapPage queryLayer2Topology(final long nodeId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_QUERY_L2_TOPOLOGY);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
		sendMessage(msg);

		final NXCPMessage response = waitForRCC(msg.getMessageId());

		int count = response.getVariableAsInteger(NXCPCodes.VID_NUM_OBJECTS);
		long[] idList = response.getVariableAsUInt32Array(NXCPCodes.VID_OBJECT_LIST);
		if (idList.length != count)
			throw new NXCException(RCC_INTERNAL_ERROR);

		NXCMapPage page = new NXCMapPage();
		for(int i = 0; i < count; i++)
			page.addObject(new NXCMapObjectData(idList[i]));

		count = response.getVariableAsInteger(NXCPCodes.VID_NUM_LINKS);
		long varId = NXCPCodes.VID_OBJECT_LINKS_BASE;
		for(int i = 0; i < count; i++, varId += 5)
		{
			long obj1 = response.getVariableAsInt64(varId++);
			long obj2 = response.getVariableAsInt64(varId++);
			int type = response.getVariableAsInteger(varId++);
			String port1 = response.getVariableAsString(varId++);
			String port2 = response.getVariableAsString(varId++);
			page.addLink(new NXCMapObjectLink(type, obj1, obj2, port1, port2));
		}
		return page;
	}

	/**
	 * Execute action on remote agent
	 * 
	 * @param nodeId
	 *           Node object ID
	 * @param action
	 *           Action name
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void executeAction(final long nodeId, final String action) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_EXECUTE_ACTION);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
		msg.setVariable(NXCPCodes.VID_ACTION_NAME, action);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Get list of server jobs
	 * 
	 * @return list of server jobs
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public NXCServerJob[] getServerJobList() throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_JOB_LIST);
		sendMessage(msg);

		final NXCPMessage response = waitForRCC(msg.getMessageId());

		int count = response.getVariableAsInteger(NXCPCodes.VID_JOB_COUNT);
		NXCServerJob[] jobList = new NXCServerJob[count];
		long baseVarId = NXCPCodes.VID_JOB_LIST_BASE;
		for(int i = 0; i < count; i++, baseVarId += 10)
			jobList[i] = new NXCServerJob(response, baseVarId);

		return jobList;
	}

	/**
	 * Cancel server job
	 * 
	 * @param jobId
	 *           Job ID
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void cancelServerJob(long jobId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_CANCEL_JOB);
		msg.setVariableInt32(NXCPCodes.VID_JOB_ID, (int) jobId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Deploy policy on agent
	 * 
	 * @param policyId
	 *           Policy object ID
	 * @param nodeId
	 *           Node object ID
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NXCException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public void deployAgentPolicy(final long policyId, final long nodeId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DEPLOY_AGENT_POLICY);
		msg.setVariableInt32(NXCPCodes.VID_POLICY_ID, (int) policyId);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int) nodeId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
}
