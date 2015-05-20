package com.spinbit.android.arduniobluetoothtwoways2;

import android.app.Fragment;

/**
 * Created by isaacperry on 5/9/15.
 */
public class SpinListActivity extends SingleFragmentActivity {
    @Override
    protected Fragment createFragment() {
        return SpinListFragment.newInstance();
    }
}
