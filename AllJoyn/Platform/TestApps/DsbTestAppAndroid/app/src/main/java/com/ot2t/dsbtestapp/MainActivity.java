package com.opent2t.dsbtestapp;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import com.opent2t.bridge.Bridge;
import com.opent2t.bridge.DeviceInfo;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        Bridge.initialize();

        FloatingActionButton fabOn = (FloatingActionButton) findViewById(R.id.fabOn);
        fabOn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                DeviceInfo device = new DeviceInfo();
                device.name = "My test device";
                device.vendor = "Samsung SmartThings";
                device.model = "light";
                device.props = "{\"AuthToken\": \"d26e46dd-e8e3-4048-9164-3cb3c79d7dac\", \"ControlId\": \"81d21115-cf03-4ebb-8517-7b5e851bf0d9\", \"ControlString\": \"/switch\" }";
                // Load the schema
                InputStream baseXmlStream = getResources().openRawResource(R.raw.device_schema);
                String baseXml = readTextFile(baseXmlStream);
                // Load the javascript
                InputStream scriptStream = getResources().openRawResource(R.raw.device_script);
                String script = readTextFile(scriptStream);
                String modulesPath = "<insert modules path here>";

                Bridge.addDevice(device, baseXml, script, modulesPath);

                Snackbar.make(view, "Turned on the lights", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        FloatingActionButton fabOff = (FloatingActionButton) findViewById(R.id.fabOff);
        fabOff.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Turned off the lights", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
    }

    @Override
    protected void onDestroy() {
        Bridge.shutdown();
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private String readTextFile(InputStream inputStream) {
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

        byte buf[] = new byte[1024];
        int len;
        try {
            while ((len = inputStream.read(buf)) != -1) {
                outputStream.write(buf, 0, len);
            }
            outputStream.close();
            inputStream.close();
        }
        catch (IOException e) {
        }

        return outputStream.toString();
    }
}
