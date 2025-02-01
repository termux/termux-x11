package com.termux.x11;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.SeekBar;
import android.widget.Toast;
import android.widget.EditText;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.x11.input.VirtualKeyHandler;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;


public class VirtualKeyMapperActivity extends AppCompatActivity {
    private FrameLayout buttonContainer;
    private int lastButtonId = 0; // ID pentru butoane noi

    private static final int BUTTON_NAME_TAG = 1001; // Un ID unic pentru tag-ul numelui butonului
    private View selectedButton; // Butonul selectat în meniul contextual

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_virtual_key_mapper);

        buttonContainer = findViewById(R.id.buttonContainer);
        Button addNewKeyButton = findViewById(R.id.addNewKeyButton);
        Button savePresetButton = findViewById(R.id.savePresetButton);
        Button loadPresetButton = findViewById(R.id.loadPresetButton);

        // Când apasă pe "Add New Key", se creează un buton nou
        addNewKeyButton.setOnClickListener(v -> addNewButton(null));

        // Când apasă pe "Save Preset", salvează toate butoanele
        savePresetButton.setOnClickListener(v -> savePreset());

        // Când apasă pe "Load Preset", deschide lista cu preseturi
        VirtualKeyHandler virtualKeyHandler = new VirtualKeyHandler(this);
        loadPresetButton.setOnClickListener(v -> showLoadPresetDialog());
        //loadButtons(this, virtualKeyHandler);
        //loadPreset("default");


        buttonContainer = findViewById(R.id.buttonContainer);
        if (buttonContainer == null) {
            Log.e("DEBUG", "buttonContainer NU a fost găsit!"); // Dacă apare asta, verifică ID-ul din XML
        }
        buttonContainer.setVisibility(View.VISIBLE);


    }


    /**
     * Adaugă un buton nou pe ecran
     */
    private void addNewButton(Button button) {
        if (button == null) {
            Button newButton = new Button(this);
            int newId = generateUniqueButtonId();  // ID unic
            newButton.setId(newId);

            newButton.setText("Key " + newId);
            newButton.setLayoutParams(new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
            ));
            button = newButton;
        }

        // Înregistrează context menu & drag
        registerForContextMenu(button);
        enableDrag(button);

        // OnLongClick → deschide meniul contextual
        button.setOnLongClickListener(v -> {
            openContextMenu(v);
            return true;
        });

        buttonContainer.addView(button);
    }



    /**
     * Permite mutarea butoanelor
     */
    // Permite mutarea și afișează meniul contextual la long press
    private void enableDrag(View view) {
        view.setOnTouchListener(new View.OnTouchListener() {
            private float dX, dY;
            private long touchStartTime;
            private static final int LONG_PRESS_THRESHOLD = 500;
            private Handler longPressHandler = new Handler();
            private Runnable longPressRunnable = () -> view.performLongClick();

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        dX = v.getX() - event.getRawX();
                        dY = v.getY() - event.getRawY();
                        touchStartTime = System.currentTimeMillis();
                        longPressHandler.postDelayed(longPressRunnable, LONG_PRESS_THRESHOLD);
                        break;

                    case MotionEvent.ACTION_MOVE:
                        float deltaX = Math.abs(event.getRawX() - (v.getX() - dX));
                        float deltaY = Math.abs(event.getRawY() - (v.getY() - dY));

                        if (deltaX > 10 || deltaY > 10) {
                            longPressHandler.removeCallbacks(longPressRunnable);
                        }

                        v.setX(event.getRawX() + dX);
                        v.setY(event.getRawY() + dY);

                        // Salvăm poziția în timp real
                        if (v instanceof Button) {
                            saveButtonSettings((Button) v);
                        }
                        break;

                    case MotionEvent.ACTION_UP:
                        longPressHandler.removeCallbacks(longPressRunnable);
                        if (v instanceof Button) {
                            saveButtonSettings((Button) v);
                        }
                        break;
                }
                return true;
            }
        });

        registerForContextMenu(view);
    }


    private int generateUniqueButtonId() {
        SharedPreferences prefs = getSharedPreferences("button_prefs", MODE_PRIVATE);
        int lastUsedId = prefs.getInt("lastUsedId", 0);
        int newId = lastUsedId + 1;

        // Salvăm noul ID
        prefs.edit().putInt("lastUsedId", newId).apply();

        return newId;
    }



    /**
     * Salvează poziția butonului în SharedPreferences
     *
     * @return
     */
    private String saveButtonSettings(Button button) {
        SharedPreferences prefs = getSharedPreferences("button_prefs", MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        Set<String> buttonData = new HashSet<>(prefs.getStringSet("button_data", new HashSet<>()));

        int id = button.getId();
        String tag = (String) button.getTag();
        String text = button.getText().toString();
        float x = button.getX();
        float y = button.getY();
        int width = button.getWidth();
        int height = button.getHeight();
        int alpha = button.getBackground().getAlpha();; // Salvăm tag-ul (inputul asociat)

        // Construim un string care conține toate datele
        String line = id + "," + x + "," + y + "," + width + "," + height + "," + alpha + "," + tag + "," + text;

        // Ștergem orice linie care începe cu același ID, ca să nu avem duplicat
        buttonData.removeIf(data -> data.startsWith(id + ","));

        // Adăugăm noua linie
        buttonData.add(line);

        editor.putStringSet("button_data", buttonData);
        editor.apply();

        Log.d("DEBUG", "Salvăm butonul: id=" + id + ", tag=" + tag + ", text=" + text);
        return line;
    }



    /**
     * Încarcă butoanele salvate
     */
    public static List<Button> loadButtons(Context context, VirtualKeyHandler virtualKeyHandler) {
        SharedPreferences prefs = context.getSharedPreferences("button_prefs", MODE_PRIVATE);
        Set<String> buttonData = prefs.getStringSet("button_data", new HashSet<>());

        List<Button> buttons = new ArrayList<>();

        for (String data : buttonData) {
            String[] parts = data.split(",");
            if (parts.length < 8) continue; // Verificăm dacă avem toate datele (inclusiv tag-ul și textul)

            int id = Integer.parseInt(parts[0]);
            float x = Float.parseFloat(parts[1]);
            float y = Float.parseFloat(parts[2]);
            int width = Integer.parseInt(parts[3]);
            int height = Integer.parseInt(parts[4]);
            int alpha = Integer.parseInt(parts[5]);  // ✅ Convertim alpha corect (0-255)
            String tag = parts[6];
            String text = parts[7];

            Button button = new Button(context);
            button.setText(text);
            button.setTag(tag);
            button.setId(id);
            button.setLayoutParams(new FrameLayout.LayoutParams(width, height));
            button.setX(x);
            button.setY(y);
            button.getBackground().setAlpha(alpha);

            // Adăugăm butonul în listă
            buttons.add(button);
        }

        Log.d("DEBUG", "✅ Toate butoanele virtuale au fost încărcate!");
        return buttons;
    }


    /**
     * Creează meniul contextual
     */
    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        getMenuInflater().inflate(R.menu.button_options_menu, menu);
        menu.setHeaderTitle("Button Options");

        // Setăm referința butonului apăsat
        selectedButton = v;
    }




    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (selectedButton instanceof Button) {
            Button button = (Button) selectedButton;

            if (item.getItemId() == R.id.action_delete) {
                buttonContainer.removeView(button);
                removeButtonFromStorage(button.getId()); // Ștergem din preferințe
                selectedButton = null;
                return true;

            } else if (item.getItemId() == R.id.action_transparency) {
                showTransparencyDialog(button); // Aplicăm transparența
                //saveButtonSettings(button);
                return true;

            } else if (item.getItemId() == R.id.action_resize) {
                showResizeDialog(button);
                return true;

            } else if (item.getItemId() == R.id.action_rename) { // Adăugăm redenumirea
                showRenameDialog(button);
                return true;
            } else if (item.getItemId() == R.id.action_set_input) {
                showSetInputDialog(button);
                return true;
            }

        }
        return super.onContextItemSelected(item);
    }



    private void removeButtonFromStorage(int buttonId) {
        SharedPreferences prefs = getSharedPreferences("button_prefs", MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        Set<String> buttonData = new HashSet<>(prefs.getStringSet("button_data", new HashSet<>()));
        buttonData.removeIf(pos -> pos.startsWith(buttonId + ","));

        editor.putStringSet("button_data", buttonData);
        editor.apply();
    }



    private void showResizeDialog(Button button) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Resize Button");

        // Adaugă un slider pentru mărime
        SeekBar sizeSeekBar = new SeekBar(this);
        sizeSeekBar.setMax(500);
        sizeSeekBar.setMin(50);
        sizeSeekBar.setProgress(button.getWidth());

        builder.setView(sizeSeekBar);

        builder.setPositiveButton("Apply", (dialog, which) -> {
            int newSize = sizeSeekBar.getProgress();
            button.setLayoutParams(new FrameLayout.LayoutParams(newSize, newSize));
            saveButtonSettings(button);
        });

        builder.setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss());

        builder.show();
    }


    private void showTransparencyDialog(Button button) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Adjust Transparency");

        // Slider pentru transparență
        SeekBar transparencySeekBar = new SeekBar(this);
        transparencySeekBar.setMax(255);
        transparencySeekBar.setMin(5);
        transparencySeekBar.setProgress(button.getBackground().getAlpha());

        builder.setView(transparencySeekBar);

        builder.setPositiveButton("Apply", (dialog, which) -> {
            int alpha = transparencySeekBar.getProgress();
            button.getBackground().setAlpha(alpha);
        });

        builder.setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss());

        builder.show();
    }

    private void showRenameDialog(Button button) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Rename Key");

        // Creăm un input pentru a introduce noul nume
        EditText input = new EditText(this);
        input.setText(button.getText().toString()); // Setăm textul actual
        builder.setView(input);

        // Buton "Apply" pentru confirmare
        builder.setPositiveButton("Apply", (dialog, which) -> {
            String newName = input.getText().toString().trim();
            if (!newName.isEmpty()) {
                button.setText(newName); // Aplică noul nume pe buton
                button.setTag("button_name"); // Salvează numele butonului

                // Salvăm și în SharedPreferences
                saveButtonSettings(button);
            }
        });

        // Buton "Cancel" pentru a ieși fără să salvăm
        builder.setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss());

        builder.show();
    }


    private void savePreset() {
        SharedPreferences prefs = getSharedPreferences("button_prefs", MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        Set<String> buttonData = new HashSet<>();
        for (int i = 0; i < buttonContainer.getChildCount(); i++) {
            View view = buttonContainer.getChildAt(i);
            if (view instanceof Button) {
                Button button = (Button) view;

                String data = saveButtonSettings(button);
                buttonData.add(data);
            }
        }


        editor.putStringSet("preset_default", buttonData);
        editor.apply();
        Toast.makeText(this, "✅ Preset saved!", Toast.LENGTH_SHORT).show();
    }


    private void showLoadPresetDialog() {
        SharedPreferences prefs = getSharedPreferences("button_prefs", MODE_PRIVATE);
        Set<String> presetKeys = prefs.getAll().keySet();

        // Filtrăm doar preset-urile
        Set<String> presetNames = new HashSet<>();
        for (String key : presetKeys) {
            if (key.startsWith("preset_")) {
                presetNames.add(key.replace("preset_", ""));
            }
        }

        if (presetNames.isEmpty()) {
            Toast.makeText(this, "No presets available", Toast.LENGTH_SHORT).show();
            return;
        }

        String[] presetArray = presetNames.toArray(new String[0]);

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Load Preset")
                .setItems(presetArray, (dialog, which) -> loadPreset(presetArray[which]))
                .setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss())
                .show();
    }


    private void showSetInputDialog(Button button) {
        String[] keys = {
                // Litere
                "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",

                // Cifre
                "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",

                // Taste speciale
                "Space", "Enter", "Backspace", "Tab", "Escape", "Delete", "Insert",
                "Home", "End", "Page Up", "Page Down",

                // Taste de navigare (săgeți)
                "↑", "↓", "←", "→",

                // Combinații de taste (modificatori)
                "Alt + F4",

                // Altele
                "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",

                // Input personalizat
                //"Custom..."
        };

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Select Input Key")
                .setItems(keys, (dialog, which) -> {
                    String selectedKey = keys[which];
                    button.setTag(selectedKey); // Salvăm inputul în tag-ul butonului
                    button.setText(selectedKey); // Setăm textul butonului
                    saveButtonSettings(button);
                })
                .setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss())
                .show();
    }



    private void loadPreset(String presetName) {
        SharedPreferences prefs = getSharedPreferences("button_prefs", MODE_PRIVATE);
        Set<String> buttonData = prefs.getStringSet("preset_" + presetName, new HashSet<>());

        if (buttonData.isEmpty()) {
            Toast.makeText(this, "Preset is empty", Toast.LENGTH_SHORT).show();
            return;
        }

        buttonContainer.removeAllViews();

        for (int i = 0; i < buttonContainer.getChildCount(); i++) {
            View view = buttonContainer.getChildAt(i);

            removeButtonFromStorage(view.getId());
        }

        for (String data : buttonData) {
            String[] parts = data.split(",");
            if (parts.length < 8) continue;

            int id = Integer.parseInt(parts[0]);
            float x = Float.parseFloat(parts[1]);
            float y = Float.parseFloat(parts[2]);
            int width = Integer.parseInt(parts[3]);
            int height = Integer.parseInt(parts[4]);
            int alpha = Integer.parseInt(parts[5]);  // ✅ Convertim alpha corect (0-255)
            String tag = parts[6];
            String text = parts[7];

            Button button = new Button(this);
            button.setText(text);
            button.setTag(tag);
            button.setId(id);
            button.setLayoutParams(new FrameLayout.LayoutParams(width, height));
            button.setX(x);
            button.setY(y);
            button.getBackground().setAlpha(alpha);  // ✅ Aplicăm corect transparența

            addNewButton(button);
        }

        Toast.makeText(this, "Preset loaded: " + presetName, Toast.LENGTH_SHORT).show();
    }


}
