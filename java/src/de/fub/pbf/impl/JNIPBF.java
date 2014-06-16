package de.fub.pbf.impl;

import de.fub.pbf.PBF;
import de.fub.pbf.util.JNIUtils;

import java.io.IOException;

public class JNIPBF implements PBF {

    protected static JNIPBF instance;

    private JNIPBF() {/* prevent instantiation */}

    public static JNIPBF getInstance() throws JNIInitializationException{
        if(instance == null) {
            setUp();
            instance = new JNIPBF();
        }
        return instance;
    }

    protected static void setUp() throws JNIInitializationException{
        try {
            JNIUtils.loadLibraryFromJar("libpbf_jni.jnilib");
        } catch (IOException e) {
            throw new JNIInitializationException(e.getMessage());
        }
    }

    @Override
    public void display() {
        _display();
    }

    @Override
    public void zoom(float factor) {
        _zoom(factor);
    }

    @Override
    public void rotate(float x, float y) {
        _rotate(x,y);
    }

    @Override
    public void move(float x, float y) {
        _move(x,y);
    }

    @Override
    public void resized(int w, int h) {
        _resized(w,h);
    }

    @Override
    public void start() throws RuntimeException {
        _start();
    }

    @Override
    public void stop() {
        _stop();
    }

    @Override
    public void init() throws java.lang.Exception {
        _init();
    }

    @Override
    public void dispose() {
        _dispose();
    }

    public static class Exception extends java.lang.Exception {
        protected Exception(String message) {
            super(message);
        }
    }

    public static class JNIInitializationException extends Exception {
        public JNIInitializationException(String message) {
            super("Could not initialize JNI: "+message);
        }
    }

    /* native methods */
    private native boolean _resized(int w, int h);
    private native boolean _init();
    private native boolean _display();

    private native boolean _start();
    private native boolean _stop();

    private native boolean _zoom(float factor);
    private native boolean _move(float x, float y);
    private native boolean _rotate(float x, float y);
    private native boolean _dispose();

}
