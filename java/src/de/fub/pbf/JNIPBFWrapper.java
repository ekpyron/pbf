package de.fub.pbf;

public class JNIPBFWrapper {

    private native boolean _resized(int w, int h);
    private native boolean _init();
    private native boolean _display();

    private native boolean _start();
    private native boolean _stop();

    private native boolean _zoom(float factor);
    private native boolean _move(float x, float y);
    private native boolean _rotate(float x, float y);

    public native void dispose();

    public void display() {
        if(!_display()) {
            throw new RuntimeException("pbf error!");
        }
    }

    public void zoom(float factor) {
        if(!_zoom(factor)) {
            throw new RuntimeException("pbf error");
        }
    }

    public void rotate(float x, float y) {
        if(!_rotate(x,y)) {
            throw new RuntimeException("pbf error");
        }
    }

    public void move(float x, float y) {
        if(!_move(x, y)) {
            throw new RuntimeException("pbf error");
        }
    }

    public void resized(int w, int h) {
        if(!_resized(w,h)) {
            throw new RuntimeException("pbf error");
        }
    }

    public void start() throws RuntimeException{
        if(!_start()) {
            throw new RuntimeException("Was not initialized");
        }
    }

    public void stop() {
        if(!_stop()) {
            throw new RuntimeException("pbf error");
        }
    }

    public void init () throws Exception {
        if(!_init()) {
            throw new Exception("Could not initialize pbf!");
        }
    }

}
