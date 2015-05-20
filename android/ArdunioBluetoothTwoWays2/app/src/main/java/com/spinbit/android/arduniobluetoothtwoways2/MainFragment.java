package com.spinbit.android.arduniobluetoothtwoways2;

import android.app.Fragment;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.Set;
import java.util.UUID;

/*
 * Created by isaacperry on 5/10/15.
 */
public class MainFragment extends Fragment {

    //************************************876   uAS*****************************************
    // Views
    //*****************************************************************************
    private Button mViewHistoryButton;
    private Button mLogButton;
    private TextView mResponseTextView;

    //*****************************************************************************
    //*****************************************************************************

    //*****************************************************************************
    // Bluetooth Entities
    //*****************************************************************************
    private Handler bluetoothHandler;
    private static final String TAG = "bluetooth2";
    final int RECIEVE_MESSAGE = 1;        // Status  for Handler
    private BluetoothAdapter btAdapter = null;
    private BluetoothSocket btSocket = null;
    private StringBuilder sb = new StringBuilder();

    private ConnectedThread mConnectedThread;

    // SPP UUID service
    private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    // MAC-address of Bluetooth module (you must edit this line)
    // private static String address = "00:06:66:74:80:AE";
    //*****************************************************************************
    //*****************************************************************************

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // save this instance when it would otherwise be destroyed: preserve bluetooth status
        setRetainInstance(true);
        //*****************************************************************************
        // Initialize Bluetooth
        //*****************************************************************************
        bluetoothHandler = new BluetoothHandler();

        btAdapter = BluetoothAdapter.getDefaultAdapter();       // get Bluetooth adapter
        checkBTState();
        initiateConnection();
        //*****************************************************************************
        //*****************************************************************************
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup parent, Bundle savedInstanceState) {
        // inflate the view
        View v = inflater.inflate(R.layout.fragment_main, parent, false);

        //*****************************************************************************
        // Wiring up the view objects
        //*****************************************************************************
        mViewHistoryButton = (Button) v.findViewById(R.id.viewHistory);
        mViewHistoryButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent i = new Intent(getActivity(), SpinListActivity.class);
                startActivity(i);
            }
        });

        mLogButton = (Button) v.findViewById(R.id.logButton);
        mLogButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mConnectedThread.write("R");
            }
        });

        mResponseTextView = (TextView) v.findViewById(R.id.responseTextView);

        //*****************************************************************************
        //*****************************************************************************

        return v;
    }

    //*****************************************************************************
    // Other Fragment Lifecycle Methods
    //*****************************************************************************
    @Override
    public void onPause() {
        super.onPause();
        DanceHistory.get(getActivity()).saveHistory();
    }
    //*****************************************************************************
    //*****************************************************************************

    //*****************************************************************************
    // Bluetooth Helper Methods and Classes
    //*****************************************************************************
    private void initiateConnection() {
        Log.d(TAG, "...initiateConnection() - try connect...");

        // Set up a pointer to the remote node using it's address.
        BluetoothDevice device = null;
        // = btAdapter.getRemoteDevice(address);

        Set<BluetoothDevice> devices = btAdapter.getBondedDevices();


        for (BluetoothDevice b : devices) {
            if(b.getName().equals("E2H")) {
                device = b;
            }

        }


        // Two things are needed to make a connection:
        //   A MAC address, which we got above.
        //   A Service ID or UUID.  In this case we are using the
        //     UUID for SPP.

        Log.d(TAG, "creating a new socket");
        try {
            btSocket = createBluetoothSocket(device);
        } catch (IOException e) {
            errorExit("Fatal Error", "In onResume() and socket create failed: " + e.getMessage() + ".");
        }


        // Discovery is resource intensive.  Make sure it isn't going on
        // when you attempt to connect and pass your message.
        btAdapter.cancelDiscovery();

        // Establish the connection.  This will block until it connects.
        Log.d(TAG, "...Connecting...");

        try {
            btSocket.connect();
            Log.d(TAG, "....Connection ok...");
        } catch (IOException e) {
            try {
                btSocket.close();
            } catch (IOException e2) {
                errorExit("Fatal Error", "In onResume() and unable to close socket during connection failure" + e2.getMessage() + ".");
            }
        }

        // Create a data stream so we can talk to server.
        Log.d(TAG, "...Create Socket...");

        mConnectedThread = new ConnectedThread(btSocket);
        mConnectedThread.start();
    }

    private void cancelConnection() {
        Log.d(TAG, "...In cancelConnection...");

        try     {
            btSocket.close();
        } catch (IOException e2) {
            errorExit("Fatal Error", "In onPause() and failed to close socket." + e2.getMessage() + ".");
        }
    }


    private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        if(Build.VERSION.SDK_INT >= 10){
            try {
                final Method m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", new Class[] { UUID.class });
                return (BluetoothSocket) m.invoke(device, MY_UUID);
            } catch (Exception e) {
                Log.e(TAG, "Could not create Insecure RFComm Connection", e);
            }
        }
        return  device.createRfcommSocketToServiceRecord(MY_UUID);
    }

    private void checkBTState() {
        // Check for Bluetooth support and then check to make sure it is turned on
        // Emulator doesn't support Bluetooth and will return null
        if(btAdapter==null) {
            errorExit("Fatal Error", "Bluetooth not support");
        } else {
            if (btAdapter.isEnabled()) {
                Log.d(TAG, "...Bluetooth ON...");
            } else {
                //Prompt user to turn on Bluetooth
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, 1);
            }
        }
    }

    private void errorExit(String title, String message){
        Toast.makeText(getActivity().getBaseContext(), title + " - " + message, Toast.LENGTH_LONG).show();
        getActivity().finish();
    }

    private class ConnectedThread extends Thread {
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            // Get the input and output streams, using temp objects because
            // member streams are final
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            byte[] buffer = new byte[256];  // buffer store for the stream
            int bytes; // bytes returned from read()

            // Keep listening to the InputStream until an exception occurs
            while (true) {
                try {
                    // Read from the InputStream
                    bytes = mmInStream.read(buffer);        // Get number of bytes and message in "buffer"
                    bluetoothHandler.obtainMessage(RECIEVE_MESSAGE, bytes, -1, buffer).sendToTarget();     // Send to message queue Handler
                } catch (IOException e) {
                    break;
                }
            }
        }

        /* Call this from the main activity to send data to the remote device */
        public void write(String message) {
            Log.d(TAG, "...Data to send: " + message + "...");
            byte[] msgBuffer = message.getBytes();
            try {
                mmOutStream.write(msgBuffer);
            } catch (IOException e) {
                Log.d(TAG, "...Error data send: " + e.getMessage() + "...");
            }
        }
    }


    private class BluetoothHandler extends Handler {
        public BluetoothHandler() {
        }

        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "Handling message");
            switch (msg.what) {
                case RECIEVE_MESSAGE:                                                   // if receive massage
                    byte[] readBuf = (byte[]) msg.obj;
                    String strIncom = new String(readBuf, 0, msg.arg1);                 // create string from bytes array
                    sb.append(strIncom);                                                // append string
                    int endOfLineIndex = sb.indexOf("\r\n");                            // determine the end-of-line
                    if (endOfLineIndex > 0) {                                            // if end-of-line,
                        String sbprint = sb.substring(0, endOfLineIndex);               // extract string
                        sb.delete(0, sb.length());                                      // and clear
                        mResponseTextView.setText("Data from Arduino: " + sbprint);            // update TextView
                        int spins = Integer.parseInt(sbprint);
                        DanceSession newSession = new DanceSession(spins);
                        DanceHistory.get(getActivity()).addDanceSession(newSession);
                    }
                    //Log.d(TAG, "...String:"+ sb.toString() +  "Byte:" + msg.arg1 + "...");
                    break;
            }
        }
    }
    //*****************************************************************************
    //*****************************************************************************

    //*****************************************************************************
    // Fragment
    //*****************************************************************************
    public static MainFragment newInstance() {
        Bundle args = new Bundle();

        MainFragment fragment = new MainFragment();
        fragment.setArguments(args);

        return fragment;
    }
    //*****************************************************************************
    //*****************************************************************************
}
