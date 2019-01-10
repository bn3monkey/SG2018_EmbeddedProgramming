package javadriver;

public class JavaDriver
{
	public static native int driver_open();
	public static native int driver_close(int fd);
	public static native int driver_read(int fd); 
	public static native int driver_write(int fd, int msg);
	public static native int driver_ioctl(int fd, int msg);

	static
	{
		System.loadLibrary("JavaDriver");
	};
}