package com.spinbit.android.arduniobluetoothtwoways2;

import android.app.ListFragment;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;

/**
 * Created by isaacperry on 5/9/15.
 */
// maintains the list view for the data saved by this app
public class SpinListFragment extends ListFragment{
    // list of data
    private ArrayList<DanceSession> mHistory;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActivity().setTitle(R.string.history_title);
        // get the data from the standalone
        mHistory = DanceHistory.get(getActivity()).getHistory();
        // instantiate a new adapter and set it to the adapter used by this fragment
        SpinAdapter adapter = new SpinAdapter(mHistory);
        setListAdapter(adapter);
    }


    public static SpinListFragment newInstance() {
        Bundle args = new Bundle();

        SpinListFragment fragment = new SpinListFragment();
        fragment.setArguments(args);

        return fragment;
    }

    // custom adapter used to apply a custom view to the list items
    private class SpinAdapter extends ArrayAdapter<DanceSession> {
        public SpinAdapter(ArrayList<DanceSession> history) {
            super(getActivity(), 0, history);
        }

        @Override
        // applies the data saved by the DanceSessions in the list to this list view with a custom layout
        // for each item
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = getActivity().getLayoutInflater().inflate(R.layout.list_item_spin, null);
            }

            DanceSession d = getItem(position);

            TextView countTextView = (TextView)convertView.findViewById(R.id.spin_list_item_countTextView);
            countTextView.setText(d.getSpinCount() + " spins");
            TextView dateTextView = (TextView)convertView.findViewById(R.id.spin_list_item_dateTextView);
            dateTextView.setText(d.getDate().toString());

            return convertView;
        }
    }
}
