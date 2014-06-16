package de.fub.pbf;

public interface PBF {
    public void display();
    public void zoom(float factor);
    public void rotate(float x, float y);
    public void move(float x, float y);
    public void resized(int w, int h);
    public void start() throws RuntimeException;
    public void stop();
    public void init () throws Exception;
    public void dispose();

}
