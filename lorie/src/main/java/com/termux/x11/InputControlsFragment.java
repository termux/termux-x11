package com.termux.x11;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.preference.PreferenceManager;

import com.termux.x11.inputcontrols.ControlsProfile;
import com.termux.x11.inputcontrols.ExternalController;
import com.termux.x11.inputcontrols.InputControlsManager;
import com.termux.x11.widget.InputControlsView;

import org.json.JSONObject;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

public class InputControlsFragment extends Fragment {
    public static final int OPEN_FILE_REQUEST_CODE = 8341;

    public interface OnProfileSelectedListener {
        void onProfileSelected(ControlsProfile profile);
    }

    private InputControlsManager manager;
    private ControlsProfile currentProfile;
    private Runnable updateLayout;
    private final int selectedProfileId;
    private OnProfileSelectedListener listener;

    public InputControlsFragment(int selectedProfileId) {
        this.selectedProfileId = selectedProfileId;
    }

    public void setOnProfileSelectedListener(OnProfileSelectedListener listener) {
        this.listener = listener;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        manager = new InputControlsManager(requireContext());
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (requestCode == OPEN_FILE_REQUEST_CODE && resultCode == Activity.RESULT_OK && data != null) {
            try (InputStream is = requireContext().getContentResolver().openInputStream(data.getData())) {
                byte[] bytes = new byte[is.available()];
                //noinspection ResultOfMethodCallIgnored
                is.read(bytes);
                ControlsProfile importedProfile = manager.importProfile(new JSONObject(new String(bytes, StandardCharsets.UTF_8)));
                if (importedProfile != null) {
                    currentProfile = importedProfile;
                    setProfile(importedProfile);
                }
                else Toast.makeText(getContext(), R.string.lorie_input_controls_unable_to_import_profile, Toast.LENGTH_SHORT).show();
            }
            catch (Exception e) {
                Toast.makeText(getContext(), R.string.lorie_input_controls_unable_to_import_profile, Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.lorie_input_controls_fragment, container, false);
        final Context context = requireContext();
        final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);

        currentProfile = selectedProfileId > 0 ? manager.getProfile(selectedProfileId) : null;

        final Spinner sProfile = view.findViewById(R.id.SProfile);
        loadProfileSpinner(sProfile);

        final SeekBar sbCursorSpeed = view.findViewById(R.id.SBCursorSpeed);
        final TextView tvCursorSpeedValue = view.findViewById(R.id.TVCursorSpeedValue);
        sbCursorSpeed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvCursorSpeedValue.setText((progress + 10) + "%");
                if (fromUser && currentProfile != null) {
                    currentProfile.setCursorSpeed((progress + 10) / 100.0f);
                    currentProfile.save();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        CheckBox cbDisableMouseInput = view.findViewById(R.id.CBDisableMouseInput);
        cbDisableMouseInput.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (currentProfile != null) {
                currentProfile.setDisableMouseInput(isChecked);
                currentProfile.save();
            }
        });

        updateLayout = () -> {
            if (currentProfile != null) {
                sbCursorSpeed.setProgress(Math.round(currentProfile.getCursorSpeed() * 100) - 10);
                cbDisableMouseInput.setChecked(currentProfile.isDisableMouseInput());
            }
            else {
                sbCursorSpeed.setProgress(90);
                cbDisableMouseInput.setChecked(false);
            }
            loadExternalControllers(view);
        };

        updateLayout.run();

        SeekBar sbOverlayOpacity = view.findViewById(R.id.SBOverlayOpacity);
        TextView tvOverlayOpacityValue = view.findViewById(R.id.TVOverlayOpacityValue);
        sbOverlayOpacity.setProgress(Math.round(preferences.getFloat("overlay_opacity", InputControlsView.DEFAULT_OVERLAY_OPACITY) * 100) - 10);
        tvOverlayOpacityValue.setText((sbOverlayOpacity.getProgress() + 10) + "%");
        sbOverlayOpacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvOverlayOpacityValue.setText((progress + 10) + "%");
                if (fromUser) preferences.edit().putFloat("overlay_opacity", (progress + 10) / 100.0f).apply();
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        view.findViewById(R.id.BTAddProfile).setOnClickListener((v) -> promptText(context, R.string.lorie_input_controls_profile_name, null, (name) -> {
            currentProfile = manager.createProfile(name);
            setProfile(currentProfile);
            loadProfileSpinner(sProfile);
            updateLayout.run();
        }));

        view.findViewById(R.id.BTEditProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                promptText(context, R.string.lorie_input_controls_profile_name, currentProfile.getName(), (name) -> {
                    currentProfile.setName(name);
                    currentProfile.save();
                    loadProfileSpinner(sProfile);
                });
            }
            else Toast.makeText(context, R.string.lorie_input_controls_no_profile_selected, Toast.LENGTH_SHORT).show();
        });

        view.findViewById(R.id.BTDuplicateProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                currentProfile = manager.duplicateProfile(currentProfile);
                setProfile(currentProfile);
                loadProfileSpinner(sProfile);
                updateLayout.run();
            }
            else Toast.makeText(context, R.string.lorie_input_controls_no_profile_selected, Toast.LENGTH_SHORT).show();
        });

        view.findViewById(R.id.BTRemoveProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                new AlertDialog.Builder(context)
                        .setMessage(R.string.lorie_input_controls_remove_profile)
                        .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                            manager.removeProfile(currentProfile);
                            currentProfile = null;
                            setProfile(null);
                            loadProfileSpinner(sProfile);
                            updateLayout.run();
                        })
                        .setNegativeButton(android.R.string.cancel, null)
                        .show();
            }
            else Toast.makeText(context, R.string.lorie_input_controls_no_profile_selected, Toast.LENGTH_SHORT).show();
        });

        view.findViewById(R.id.BTImportProfile).setOnClickListener((v) -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, OPEN_FILE_REQUEST_CODE);
        });

        view.findViewById(R.id.BTExportProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                java.io.File exportedFile = manager.exportProfile(currentProfile);
                if (exportedFile != null) {
                    Toast.makeText(context, getString(R.string.lorie_input_controls_profile_exported, exportedFile.getPath()), Toast.LENGTH_LONG).show();
                }
                else Toast.makeText(context, R.string.lorie_input_controls_profile_export_failed, Toast.LENGTH_SHORT).show();
            }
            else Toast.makeText(context, R.string.lorie_input_controls_no_profile_selected, Toast.LENGTH_SHORT).show();
        });

        view.findViewById(R.id.BTControlsEditor).setOnClickListener((v) -> {
            if (currentProfile != null) {
                Intent intent = new Intent(context, ControlsEditorActivity.class);
                intent.putExtra("profile_id", currentProfile.id);
                startActivity(intent);
            }
            else Toast.makeText(context, R.string.lorie_input_controls_no_profile_selected, Toast.LENGTH_SHORT).show();
        });

        return view;
    }

    private void setProfile(ControlsProfile profile) {
        if (listener != null) listener.onProfileSelected(profile);
    }

    private void promptText(Context context, int titleResId, String initialValue, java.util.function.Consumer<String> callback) {
        final EditText editText = new EditText(context);
        editText.setInputType(InputType.TYPE_CLASS_TEXT);
        if (initialValue != null) {
            editText.setText(initialValue);
            editText.setSelection(initialValue.length());
        }
        new AlertDialog.Builder(context)
                .setTitle(titleResId)
                .setView(editText)
                .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    String value = editText.getText().toString().trim();
                    if (!value.isEmpty()) callback.accept(value);
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    @Override
    public void onStart() {
        super.onStart();
        if (updateLayout != null) updateLayout.run();
    }

    private void loadProfileSpinner(Spinner spinner) {
        final ArrayList<ControlsProfile> profiles = manager.getProfiles();
        ArrayList<String> values = new ArrayList<>();
        values.add("-- "+getString(R.string.lorie_input_controls_none)+" --");

        int selectedPosition = 0;
        for (int i = 0; i < profiles.size(); i++) {
            ControlsProfile profile = profiles.get(i);
            if (profile == currentProfile) selectedPosition = i + 1;
            values.add(profile.getName());
        }

        spinner.setAdapter(new ArrayAdapter<>(requireContext(), android.R.layout.simple_spinner_dropdown_item, values));
        spinner.setSelection(selectedPosition, false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                currentProfile = position > 0 ? profiles.get(position - 1) : null;
                setProfile(currentProfile);
                updateLayout.run();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void loadExternalControllers(final View view) {
        LinearLayout container = view.findViewById(R.id.LLExternalControllers);
        container.removeAllViews();
        Context context = requireContext();
        LayoutInflater inflater = LayoutInflater.from(context);
        ArrayList<ExternalController> connectedControllers = ExternalController.getControllers();

        ArrayList<ExternalController> controllers = currentProfile != null ? currentProfile.loadControllers() : new ArrayList<>();
        for (ExternalController controller : connectedControllers) {
            if (!controllers.contains(controller)) controllers.add(controller);
        }

        if (!controllers.isEmpty()) {
            view.findViewById(R.id.TVEmptyText).setVisibility(View.GONE);
            String bindingsText = context.getString(R.string.lorie_input_controls_bindings).toLowerCase();
            for (final ExternalController controller : controllers) {
                View itemView = inflater.inflate(R.layout.external_controller_list_item, container, false);
                ((TextView)itemView.findViewById(R.id.TVTitle)).setText(controller.getName());

                int controllerBindingCount = controller.getControllerBindingCount();
                ((TextView)itemView.findViewById(R.id.TVSubtitle)).setText(controllerBindingCount+" "+bindingsText);

                ImageView imageView = itemView.findViewById(R.id.ImageView);
                imageView.setAlpha(controller.isConnected() ? 1.0f : 0.4f);

                if (controllerBindingCount > 0) {
                    ImageButton removeButton = itemView.findViewById(R.id.BTRemove);
                    removeButton.setVisibility(View.VISIBLE);
                    removeButton.setOnClickListener((v) -> new AlertDialog.Builder(context)
                            .setMessage(R.string.lorie_input_controls_remove_controller_confirm)
                            .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                                currentProfile.removeController(controller);
                                currentProfile.save();
                                loadExternalControllers(view);
                            })
                            .setNegativeButton(android.R.string.cancel, null)
                            .show());
                }

                container.addView(itemView);
            }
        }
        else view.findViewById(R.id.TVEmptyText).setVisibility(View.VISIBLE);
    }
}
