package com.spinbit.android.arduniobluetoothtwoways2;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Date;
import java.util.UUID;

/**
 * Created by isaacperry on 5/9/15.
 */
// class that models a single session of dancing
// records the date of the session, a unique identifier, and the spin count of this session
public class DanceSession {
    // ID's used for serializing this object into JSON
    private static final String JSON_ID = "id";
    private static final String JSON_DATE = "date";
    private static final String JSON_COUNT = "count";

    // unique identifier
    private UUID mId;
    // date of this session
    private Date mDate;
    // spin count from this session
    private int mSpinCount;

    public DanceSession(int spinCount) {
        mId = UUID.randomUUID();
        mDate = new Date();
        setSpinCount(spinCount);
    }

    public DanceSession(JSONObject json) throws JSONException {
        mId = UUID.fromString(json.getString(JSON_ID));
        mDate = new Date(json.getLong(JSON_DATE));
        mSpinCount = json.getInt(JSON_COUNT);
    }

    // serializes this instance into a JSON object
    public JSONObject toJSON() throws JSONException {
        JSONObject json = new JSONObject();
        json.put(JSON_ID, mId.toString());
        json.put(JSON_DATE, mDate.getTime());
        json.put(JSON_COUNT, mSpinCount);
        return json;
    }

    public UUID getId() {
        return mId;
    }

    public void setId(UUID id) {
        mId = id;
    }

    public Date getDate() {
        return mDate;
    }

    public void setDate(Date date) {
        mDate = date;
    }

    public int getSpinCount() {
        return mSpinCount;
    }

    public void setSpinCount(int spinCount) {
        mSpinCount = spinCount;
    }

    @Override
    public String toString() {
        return "[" + mId + ", " + mDate + ", " + mSpinCount + "]";
    }
}
