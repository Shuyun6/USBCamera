package com.shuyun.pupilla;

/**
 * Created by Shuyun on 2018/1/10 0010.
 */

public class Pupilla {

    static {
        System.loadLibrary("load-lib");
    }

    public native String configuration();

}
