package com.spinbit.android.arduniobluetoothtwoways2;

import android.content.Context;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONTokener;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.util.ArrayList;

/**
 * Created by isaacperry on 5/9/15.
 */
public class SpinBitJSONSerializer {
    // gives the serializer access to this app's sandbox
    private Context mContext;
    // name of the file that JSON data will be backed up
    private String mFilename;


    public SpinBitJSONSerializer(Context c, String f) {
        mContext = c;
        mFilename = f;
    }

    // returns an ArrayList<DanceSession> that is reconstructed from the JSON back up file
    public ArrayList<DanceSession> loadHistory() throws IOException, JSONException {
        ArrayList<DanceSession> crimes = new ArrayList<DanceSession>();
        BufferedReader reader = null;
        try {
            InputStream in = mContext.openFileInput(mFilename);
            reader = new BufferedReader(new InputStreamReader(in));
            StringBuilder jsonString = new StringBuilder();
            String line = null;
            while ((line = reader.readLine()) != null) {
                jsonString.append(line);
            }

            JSONArray array = (JSONArray) new JSONTokener(jsonString.toString()).nextValue();
            for (int i = 0; i < array.length(); i++) {
                crimes.add(new DanceSession(array.getJSONObject(i)));
            }
        } catch (FileNotFoundException e) {

        } finally {
            if (reader != null) {
                reader.close();
            }
        }
        return crimes;
    }

    // backs-up the contents of history into a JSON file
    public void saveHistory(ArrayList<DanceSession> history) throws JSONException, IOException {
        JSONArray array = new JSONArray();
        for (DanceSession d : history) {
            array.put(d.toJSON());
        }
        Writer writer = null;
        try {
            OutputStream out = mContext.openFileOutput(mFilename, Context.MODE_PRIVATE);
            writer = new OutputStreamWriter(out);
            writer.write(array.toString());
        } finally {
            if (writer != null) {
                writer.close();
            }
        }
    }

}

