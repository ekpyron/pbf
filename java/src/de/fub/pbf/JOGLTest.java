package de.fub.pbf;

import com.jogamp.opengl.util.Animator;

import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLCanvas;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

public class JOGLTest extends JNIPBFWrapper implements ComponentListener, GLEventListener {

    public static void main(String[] args) {
        System.load("C:\\Users\\Moritz\\IdeaProjects\\jnitest\\libpbf_jni.jnilib");
        new JOGLTest().run();
    }

    Frame f = new Frame("test");
    GLProfile profile = GLProfile.getDefault();
    GLCapabilities capabilities = new GLCapabilities(profile);
    GLCanvas canvas = new GLCanvas(capabilities);

    public boolean isCtrl, isShift;
    public boolean running;

    protected int lastX=-1, lastY=-1;

    private void run() {
        //System.out.println(System.getProperty("java.library.path"));
        canvas.addGLEventListener(this);

        f.setSize(1280, 720);
        f.add(canvas);
        f.setVisible(true);
        f.addComponentListener(this);
        canvas.addMouseMotionListener(new MouseAdapter() {
            @Override
            public void mouseMoved(MouseEvent e) {
                lastX = e.getX();
                lastY = e.getY();
            }

            @Override
            public void mouseDragged(MouseEvent e) {
                if(lastX > -1 && lastY > -1) {
                    canvas.getContext().makeCurrent();
                    int relX = e.getX() - lastX;
                    int relY = e.getY() - lastY;
                    if(isCtrl) {
                        move(relX, relY);
                    } else if(isShift) {
                        zoom(relX - relY);
                    } else {
                        rotate (relX, relY);
                    }
                }
                lastX = e.getX();
                lastY = e.getY();
            }

            @Override
            public void mouseClicked(MouseEvent e) {
                super.mouseClicked(e);
            }
        });
        canvas.addKeyListener(new KeyAdapter() {

            @Override
            public void keyTyped(KeyEvent e) {
                switch (e.getKeyCode()) {
                    case KeyEvent.VK_SPACE:
                        running = !running;
                        if (running) {
                            start();
                        } else {
                            stop();
                        }
                }
            }

            @Override
            public void keyPressed(KeyEvent e) {
                switch (e.getKeyCode()) {
                    case KeyEvent.VK_SHIFT:
                        isShift = true;
                        break;
                    case KeyEvent.VK_CONTROL:
                        isCtrl = true;
                        break;
                }
            }

            @Override
            public void keyReleased(KeyEvent e) {
                switch (e.getKeyCode()) {
                    case KeyEvent.VK_SHIFT:
                        isShift = false;
                        break;
                    case KeyEvent.VK_CONTROL:
                        isCtrl = false;
                        break;
                    case KeyEvent.VK_SPACE:
                        running = !running;
                        if (running) {
                            start();
                        } else {
                            stop();
                        }
                        break;
                }
            }
        });
        f.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }
        });


        canvas.requestFocusInWindow();
        Animator anim = new Animator (canvas);
        anim.start ();
    }

    @Override
    public void componentResized(ComponentEvent e) {
    }

    @Override
    public void componentMoved(ComponentEvent e) {

    }

    @Override
    public void componentShown(ComponentEvent e) {

    }

    @Override
    public void componentHidden(ComponentEvent e) {

    }

    @Override
    public void init(GLAutoDrawable glAutoDrawable) {
        try {
            init();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void dispose(GLAutoDrawable glAutoDrawable) {
        dispose();
    }

    @Override
    public void display(GLAutoDrawable glAutoDrawable) {
        display();
    }

    @Override
    public void reshape(GLAutoDrawable glAutoDrawable, int i, int i2, int w, int h) {
        resized(w,h);
    }
}
