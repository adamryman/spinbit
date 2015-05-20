package com.spinbit.android.arduniobluetoothtwoways2;

import android.content.Context;
import android.util.Log;

import java.util.ArrayList;
import java.util.UUID;

/**
 * Created by isaacperry on 5/9/15.
 */
// stand alone, that acts as a data resource for this app
public class DanceHistory {

    // tag for logging
    private static final String TAG = "DanceHistory";
    // file that this library will back its self up to
    private static final String FILENAME = "crimes.json";

    // the data maintained by this library
    private ArrayList<DanceSession> mHistory;
    // serializer used to convert this library's data into JSON
    private SpinBitJSONSerializer mSerializer;

    // a reference to the only instance of this library
    private static DanceHistory sDanceHistory;
    private Context mAppContext;

    // private constructor that will generate a new library, or load an existing one from the JSON file
    private DanceHistory(Context appContext) {
        mAppContext = appContext;
        mSerializer = new SpinBitJSONSerializer(mAppContext, FILENAME);
        try {
            mHistory = mSerializer.loadHistory();
        } catch (Exception e) {
            mHistory = randomLog();
            Log.e(TAG, "Error loading history: ", e);
        }
    }

    // returns a reference to the only instance of this library
    public static DanceHistory get(Context c) {
        if (sDanceHistory == null) {
            sDanceHistory = new DanceHistory(c.getApplicationContext());
        }
        return sDanceHistory;
    }

    // returns the data currently stored by this library
    public ArrayList<DanceSession> getHistory() {
        return mHistory;
    }

    // returns the DanceSession saved by this library with the specified unique identifier
    public DanceSession getDanceSession(UUID id) {
        for (DanceSession d : mHistory) {
            if (d.getId().equals(id)) {
                return d;
            }
        }
        return null;
    }

    // appends the DanceSession to this library
    public void addDanceSession(DanceSession d) {
        mHistory.add(d);
    }

    // backs up the data in this library to the JSON file
    public boolean saveHistory() {
        try {
            mSerializer.saveHistory(mHistory);
            Log.d(TAG, "history saved to file");
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Error saving history: ", e);
            return false;
        }
    }

    // provides random data for testing/demo purposes
    private ArrayList<DanceSession> randomLog() {
        ArrayList<DanceSession> newLog = new ArrayList<DanceSession>();
        for (int i = 0; i < 15; i++) {
            newLog.add(new DanceSession(i));
        }
        return newLog;
    }
}
