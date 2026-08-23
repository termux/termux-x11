package com.termux.x11;

import android.graphics.BitmapFactory;
import android.graphics.PointF;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.NumberPicker;
import android.widget.PopupWindow;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.math.MathUtils;

import com.termux.x11.inputcontrols.Binding;
import com.termux.x11.inputcontrols.ControlElement;
import com.termux.x11.inputcontrols.ControlsProfile;
import com.termux.x11.inputcontrols.InputControlsManager;
import com.termux.x11.widget.InputControlsView;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;

public class ControlsEditorActivity extends AppCompatActivity implements View.OnClickListener {
    private InputControlsView inputControlsView;
    private ControlsProfile profile;
    private View toolbox;

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setContentView(R.layout.lorie_input_controls_editor_activity);

        inputControlsView = new InputControlsView(MainActivity.getInstance());
        inputControlsView.setEditMode(true);
        inputControlsView.setOverlayOpacity(0.6f);

        profile = InputControlsManager.loadProfile(this, ControlsProfile.getProfileFile(this, getIntent().getIntExtra("profile_id", 0)));
        ((TextView)findViewById(R.id.TVProfileName)).setText(profile.getName());
        inputControlsView.setProfile(profile);

        FrameLayout container = findViewById(R.id.FLContainer);
        container.addView(inputControlsView, 0);

        container.findViewById(R.id.BTAddElement).setOnClickListener(this);
        container.findViewById(R.id.BTRemoveElement).setOnClickListener(this);
        container.findViewById(R.id.BTElementSettings).setOnClickListener(this);

        toolbox = container.findViewById(R.id.Toolbox);

        final PointF startPoint = new PointF();
        final boolean[] isActionDown = {false};
        container.findViewById(R.id.BTMove).setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    startPoint.x = event.getX();
                    startPoint.y = event.getY();
                    isActionDown[0] = true;
                    break;
                case MotionEvent.ACTION_MOVE:
                    if (isActionDown[0]) {
                        float newX = toolbox.getX() + (event.getX() - startPoint.x);
                        float newY = toolbox.getY() + (event.getY() - startPoint.y);
                        moveToolbox(newX, newY);
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    isActionDown[0] = false;
                    break;
            }
            return true;
        });
    }

    private void moveToolbox(float x, float y) {
        final int padding = dpToPx(8);
        ViewGroup parent = (ViewGroup)toolbox.getParent();
        int width = toolbox.getWidth();
        int height = toolbox.getHeight();
        int parentWidth = parent.getWidth();
        int parentHeight = parent.getHeight();
        x = MathUtils.clamp(x, padding, parentWidth - padding - width);
        y = MathUtils.clamp(y, padding, parentHeight - padding - height);
        toolbox.setX(x);
        toolbox.setY(y);
    }

    private int dpToPx(float dp) {
        return (int)(dp * getResources().getDisplayMetrics().density);
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();
        if (id == R.id.BTAddElement) {
            if (!inputControlsView.addElement()) {
                Toast.makeText(this, R.string.lorie_input_controls_no_profile_selected, Toast.LENGTH_SHORT).show();
            }
        }
        else if (id == R.id.BTRemoveElement) {
            if (!inputControlsView.removeElement()) {
                Toast.makeText(this, R.string.lorie_input_controls_no_control_element_selected, Toast.LENGTH_SHORT).show();
            }
        }
        else if (id == R.id.BTElementSettings) {
            ControlElement selectedElement = inputControlsView.getSelectedElement();
            if (selectedElement != null) {
                showControlElementSettings(v);
            }
            else Toast.makeText(this, R.string.lorie_input_controls_no_control_element_selected, Toast.LENGTH_SHORT).show();
        }
    }

    private PopupWindow showPopupWindow(View anchor, View contentView, int widthDp) {
        PopupWindow popupWindow = new PopupWindow(this);
        popupWindow.setElevation(5.0f);
        popupWindow.setWidth(dpToPx(widthDp));
        popupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(true);
        popupWindow.setContentView(contentView);
        popupWindow.showAsDropDown(anchor);
        return popupWindow;
    }

    private void showControlElementSettings(View anchorView) {
        final ControlElement element = inputControlsView.getSelectedElement();
        final View view = LayoutInflater.from(this).inflate(R.layout.lorie_input_controls_element_settings, null);

        final Runnable updateLayout = () -> {
            ControlElement.Type type = element.getType();
            view.findViewById(R.id.LLShape).setVisibility(View.GONE);
            view.findViewById(R.id.CBToggleSwitch).setVisibility(View.GONE);
            view.findViewById(R.id.CBMouseMoveMode).setVisibility(View.GONE);
            view.findViewById(R.id.LLCustomTextIcon).setVisibility(View.GONE);
            view.findViewById(R.id.LLRangeOptions).setVisibility(View.GONE);
            view.findViewById(R.id.LLMIDIKeyOptions).setVisibility(View.GONE);
            view.findViewById(R.id.LLRadialMenuOptions).setVisibility(View.GONE);

            switch (type) {
                case BUTTON:
                    view.findViewById(R.id.LLShape).setVisibility(View.VISIBLE);
                    view.findViewById(R.id.CBToggleSwitch).setVisibility(View.VISIBLE);
                    view.findViewById(R.id.CBMouseMoveMode).setVisibility(View.VISIBLE);
                    view.findViewById(R.id.LLCustomTextIcon).setVisibility(View.VISIBLE);
                    break;
                case RANGE_BUTTON:
                    view.findViewById(R.id.LLRangeOptions).setVisibility(View.VISIBLE);
                    break;
                case MIDI_KEY:
                    view.findViewById(R.id.LLMIDIKeyOptions).setVisibility(View.VISIBLE);
                    break;
                case RADIAL_MENU:
                    ((NumberPicker)view.findViewById(R.id.NPBindings)).setValue(element.getBindingCount());
                    view.findViewById(R.id.LLRadialMenuOptions).setVisibility(View.VISIBLE);
                    break;
            }

            loadBindingSpinners(element, view);
        };

        loadTypeSpinner(element, view.findViewById(R.id.SType), updateLayout);
        loadShapeSpinner(element, view.findViewById(R.id.SShape));
        loadRangeSpinner(element, view.findViewById(R.id.SRange));
        loadNoteSpinner(element, view.findViewById(R.id.SNote));

        RadioGroup rgOrientation = view.findViewById(R.id.RGOrientation);
        rgOrientation.check(element.getOrientation() == 1 ? R.id.RBVertical : R.id.RBHorizontal);
        rgOrientation.setOnCheckedChangeListener((group, checkedId) -> {
            element.setOrientation((byte)(checkedId == R.id.RBVertical ? 1 : 0));
            profile.save();
            inputControlsView.invalidate();
        });

        NumberPicker npColumns = view.findViewById(R.id.NPColumns);
        npColumns.setMinValue(3);
        npColumns.setMaxValue(8);
        npColumns.setValue(element.getBindingCount());
        npColumns.setOnValueChangedListener((numberPicker, oldVal, value) -> {
            element.setBindingCount(value);
            profile.save();
            inputControlsView.invalidate();
        });

        NumberPicker npBindings = view.findViewById(R.id.NPBindings);
        npBindings.setMinValue(3);
        npBindings.setMaxValue(6);
        npBindings.setValue(element.getBindingCount());
        npBindings.setOnValueChangedListener((numberPicker, oldVal, value) -> {
            element.setBindingCount(value);
            loadBindingSpinners(element, view);
            profile.save();
            inputControlsView.invalidate();
        });

        SeekBar sbScale = view.findViewById(R.id.SBScale);
        TextView tvScaleValue = view.findViewById(R.id.TVScaleValue);
        sbScale.setProgress(Math.round(element.getScale() * 100) - 50);
        tvScaleValue.setText((sbScale.getProgress() + 50) + "%");
        sbScale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvScaleValue.setText((progress + 50) + "%");
                if (fromUser) {
                    element.setScale((progress + 50) / 100.0f);
                    profile.save();
                    inputControlsView.invalidate();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        SeekBar sbOpacity = view.findViewById(R.id.SBOpacity);
        TextView tvOpacityValue = view.findViewById(R.id.TVOpacityValue);
        sbOpacity.setProgress(Math.round(element.getOpacity() * 100));
        tvOpacityValue.setText(sbOpacity.getProgress() + "%");
        sbOpacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvOpacityValue.setText(progress + "%");
                if (fromUser) {
                    element.setOpacity(progress / 100.0f);
                    profile.save();
                    inputControlsView.invalidate();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        CheckBox cbToggleSwitch = view.findViewById(R.id.CBToggleSwitch);
        cbToggleSwitch.setChecked(element.isToggleSwitch());
        cbToggleSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            element.setToggleSwitch(isChecked);
            profile.save();
        });

        CheckBox cbMouseMoveMode = view.findViewById(R.id.CBMouseMoveMode);
        cbMouseMoveMode.setChecked(element.isMouseMoveMode());
        cbMouseMoveMode.setOnCheckedChangeListener((buttonView, isChecked) -> {
            element.setMouseMoveMode(isChecked);
            profile.save();
        });

        final EditText etCustomText = view.findViewById(R.id.ETCustomText);
        etCustomText.setText(element.getText());
        final LinearLayout llIconList = view.findViewById(R.id.LLIconList);
        loadIcons(llIconList, element.getIconId());

        updateLayout.run();

        PopupWindow popupWindow = showPopupWindow(anchorView, view, 340);
        popupWindow.setOnDismissListener(() -> {
            byte iconId = 0;
            if (element.getType() == ControlElement.Type.BUTTON) {
                for (int i = 0; i < llIconList.getChildCount(); i++) {
                    View child = llIconList.getChildAt(i);
                    if (child.isSelected()) {
                        iconId = (byte)child.getTag();
                        break;
                    }
                }

                String text = etCustomText.getText().toString().trim();
                element.setText(text);
            }

            element.setIconId(iconId);
            profile.save();
            inputControlsView.invalidate();
        });
    }

    private void loadTypeSpinner(final ControlElement element, Spinner spinner, final Runnable callback) {
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, ControlElement.Type.names()));
        spinner.setSelection(element.getType().ordinal(), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                ControlElement.Type newType = ControlElement.Type.values()[position];
                if (newType == element.getType()) return;
                element.setType(newType);
                profile.save();
                callback.run();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void loadShapeSpinner(final ControlElement element, Spinner spinner) {
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, ControlElement.Shape.names()));
        spinner.setSelection(element.getShape().ordinal(), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                element.setShape(ControlElement.Shape.values()[position]);
                profile.save();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void loadBindingSpinners(ControlElement element, View view) {
        LinearLayout container = view.findViewById(R.id.LLBindings);
        container.removeAllViews();

        ControlElement.Type type = element.getType();
        if (type == ControlElement.Type.BUTTON) {
            byte first = element.getFirstBindingIndex();
            for (byte i = 0, count = 0; i < element.getBindingCount(); i++) {
                if (i <= first || element.getBindingAt(i) != Binding.NONE) {
                    loadBindingSpinner(element, container, i, count++ == 0 ? R.string.lorie_input_controls_binding : 0);
                }
            }
        }
        else if (type == ControlElement.Type.D_PAD || type == ControlElement.Type.STICK || type == ControlElement.Type.TRACKPAD) {
            loadBindingSpinner(element, container, 0, R.string.lorie_input_controls_binding_up);
            loadBindingSpinner(element, container, 1, R.string.lorie_input_controls_binding_right);
            loadBindingSpinner(element, container, 2, R.string.lorie_input_controls_binding_down);
            loadBindingSpinner(element, container, 3, R.string.lorie_input_controls_binding_left);
        }
        else if (type == ControlElement.Type.RADIAL_MENU) {
            for (byte i = 0; i < element.getBindingCount(); i++) loadBindingSpinner(element, container, i, 0);
        }
    }

    private void loadBindingSpinner(final ControlElement element, final LinearLayout container, final int index, int titleResId) {
        View view = LayoutInflater.from(this).inflate(R.layout.lorie_input_controls_binding_field, container, false);

        LinearLayout titleBar = view.findViewById(R.id.LLTitleBar);
        if (titleResId > 0) {
            titleBar.setVisibility(View.VISIBLE);
            ((TextView)view.findViewById(R.id.TVTitle)).setText(titleResId);
        }
        else titleBar.setVisibility(View.GONE);

        final Spinner sBindingType = view.findViewById(R.id.SBindingType);
        final Spinner sBinding = view.findViewById(R.id.SBinding);

        ControlElement.Type type = element.getType();
        if (type == ControlElement.Type.BUTTON || type == ControlElement.Type.RADIAL_MENU) {
            ImageView addButton = view.findViewById(R.id.BTAdd);
            addButton.setVisibility(View.VISIBLE);
            addButton.setOnClickListener((v) -> {
                int nextIndex = container.getChildCount();
                if (nextIndex < element.getBindingCount()) loadBindingSpinner(element, container, nextIndex, 0);
            });
        }

        Runnable update = () -> {
            String[] bindingEntries = null;
            switch (sBindingType.getSelectedItemPosition()) {
                case 0:
                    bindingEntries = Binding.keyboardBindingLabels();
                    break;
                case 1:
                    bindingEntries = Binding.mouseBindingLabels();
                    break;
                case 2:
                    bindingEntries = Binding.gamepadBindingLabels();
                    break;
            }

            sBinding.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, bindingEntries));
            setSpinnerSelectionFromValue(sBinding, element.getBindingAt(index).toString());
        };

        sBindingType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                update.run();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        Binding selectedBinding = element.getBindingAt(index);
        if (selectedBinding.isKeyboard()) {
            sBindingType.setSelection(0, false);
        }
        else if (selectedBinding.isMouse()) {
            sBindingType.setSelection(1, false);
        }
        else if (selectedBinding.isGamepad()) {
            sBindingType.setSelection(2, false);
        }

        sBinding.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Binding binding = Binding.NONE;
                switch (sBindingType.getSelectedItemPosition()) {
                    case 0:
                        binding = Binding.keyboardBindingValues()[position];
                        break;
                    case 1:
                        binding = Binding.mouseBindingValues()[position];
                        break;
                    case 2:
                        binding = Binding.gamepadBindingValues()[position];
                        break;
                }

                if (binding != element.getBindingAt(index)) {
                    element.setBindingAt(index, binding);
                    profile.save();
                    inputControlsView.invalidate();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        update.run();
        container.addView(view);
    }

    private static boolean setSpinnerSelectionFromValue(Spinner spinner, String value) {
        spinner.setSelection(0, false);
        for (int i = 0; i < spinner.getCount(); i++) {
            if (spinner.getItemAtPosition(i).toString().equalsIgnoreCase(value)) {
                spinner.setSelection(i, false);
                return true;
            }
        }
        return false;
    }

    private void loadRangeSpinner(final ControlElement element, Spinner spinner) {
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, ControlElement.Range.names()));
        spinner.setSelection(element.getRange().ordinal(), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                element.setRange(ControlElement.Range.values()[position]);
                profile.save();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private static String[] midiNotes() {
        byte octaves = 6;
        String[] symbols = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        String[] notes = new String[symbols.length * octaves];
        int index = 0;
        for (byte i = 1; i <= octaves; i++) {
            for (String symbol : symbols) notes[index++] = symbol+i;
        }
        return notes;
    }

    private void loadNoteSpinner(final ControlElement element, Spinner spinner) {
        String[] notes = midiNotes();
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, notes));
        setSpinnerSelectionFromValue(spinner, element.getText());
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                element.setText(notes[position]);
                profile.save();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void loadIcons(final LinearLayout parent, byte selectedId) {
        ArrayList<Byte> iconIdList = new ArrayList<>();
        try {
            String[] filenames = getAssets().list("inputcontrols/icons/");
            for (String filename : filenames) {
                iconIdList.add(Byte.parseByte(filename.replaceFirst("\\.png$", "")));
            }
        }
        catch (IOException e) {}

        Byte[] iconIds = iconIdList.toArray(new Byte[0]);
        Arrays.sort(iconIds);

        int size = dpToPx(40);
        int margin = dpToPx(2);
        int padding = dpToPx(4);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(size, size);
        params.setMargins(margin, 0, margin, 0);

        for (final byte id : iconIds) {
            ImageView imageView = new ImageView(this);
            imageView.setLayoutParams(params);
            imageView.setPadding(padding, padding, padding, padding);
            imageView.setBackgroundColor(0xff303030);
            imageView.setTag(id);
            imageView.setSelected(id == selectedId);
            imageView.setOnClickListener((v) -> {
                for (int i = 0; i < parent.getChildCount(); i++) parent.getChildAt(i).setSelected(false);
                imageView.setSelected(true);
                imageView.setBackgroundColor(0xff01579b);
            });

            try (InputStream is = getAssets().open("inputcontrols/icons/"+id+".png")) {
                imageView.setImageBitmap(BitmapFactory.decodeStream(is));
            }
            catch (IOException e) {}

            parent.addView(imageView);
        }
    }
}
