package com.opent2t.bridge;

public class Bridge {
    private static Bridge s_instance;
	
    static {
        System.loadLibrary("BridgeApi");
    }
	
	public static int initialize() {
        if (s_instance == null) {
            s_instance = new Bridge();
            return s_instance.initializeBridge();
        }
		return 0;
	}
	
	public static void shutdown() {
		if (s_instance != null) {
			s_instance = null;
			System.gc();
		}
	}
	
	public static void addDevice(DeviceInfo info, String baseTypeXml, String script, String modulesPath) {
		s_instance.addDevice(info.name, info.vendor, info.model, info.version,
			info.firmwareVersion, info.serialNumber, info.description, info.props,
			baseTypeXml, script, modulesPath);
	}
	
    public void finalize() throws Throwable {
        shutdownBridge();
        super.finalize();
    }
	
	private native int initializeBridge() throws OutOfMemoryError;

	private native void shutdownBridge() throws OutOfMemoryError;
	
	private native void addDevice(String name, String vendor, String model, String version,
		String firmwareVersion, String serialNumber, String description, String props,
		String baseTypeXml, String script, String modulesPath) throws OutOfMemoryError;
}