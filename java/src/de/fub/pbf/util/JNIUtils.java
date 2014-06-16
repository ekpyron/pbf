package de.fub.pbf.util;

import com.sun.istack.internal.NotNull;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class JNIUtils {

    private JNIUtils() { /* prevent instantiation */ }

    public static enum OSType {
        WINDOWS("windows"), OSX("mac"), LINUX("linux");

        private String name;
        private OSType(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    public static void loadLibraryFromJar(@NotNull String nativeResourceName) throws IOException {
        if(!nativeResourceName.endsWith(".jnilib")) {
            nativeResourceName = System.mapLibraryName(nativeResourceName);
        }
        String path = "/native/"+getOSType().getName()+"/"+(is64Bit() ? "64" : "32")+"/"+nativeResourceName;

        String prefix = nativeResourceName.substring(0, nativeResourceName.lastIndexOf('.')+1);
        String suffix = nativeResourceName.substring(nativeResourceName.lastIndexOf('.'));

        File temp = File.createTempFile(prefix, suffix);
        temp.deleteOnExit();

        InputStream is = JNIUtils.class.getResourceAsStream(path);
        OutputStream os = new FileOutputStream(temp);

        try {
            byte[] buffer = new byte[1024];
            int readBytes;

            while ((readBytes = is.read(buffer)) != -1) {
                os.write(buffer, 0, readBytes);
            }
        } finally {
            os.close();
            is.close();
        }

        System.load(temp.getAbsolutePath());
    }

    public static OSType getOSType() {
        String osName = System.getProperty("os.name").toLowerCase();
        if(osName.contains("win")) {
            return OSType.WINDOWS;
        } else if(osName.contains("mac")) {
            return OSType.OSX;
        } else {
            return OSType.LINUX;
        }
    }

    public static boolean is64Bit() {
        boolean is64bit;
        if (System.getProperty("os.name").contains("Windows")) {
            is64bit = (System.getenv("ProgramFiles(x86)") != null);
        } else {
            is64bit = (System.getProperty("os.arch").contains("64"));
        }
        return is64bit;
    }
}