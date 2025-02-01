package com.termux.x11.input;

import android.content.Context;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.Button;
import com.termux.x11.LorieView;
import com.termux.x11.MainActivity;
import com.termux.x11.R;

public class VirtualKeyHandler {
    private Context context;

    public VirtualKeyHandler(Context context) {
        this.context = context;
    }

    public void setupInputForButton(Button button) {
        button.setOnTouchListener((v, event) -> {
            String selectedKey = (String) button.getTag();
            if (selectedKey == null) {
                Log.e("DEBUG", "⚠️ Butonul nu are un input key asignat!");
                return false;
            }

            int keyCode = getKeyEventCode(selectedKey);
            Log.d("DEBUG", "🔎 Tasta apăsată: " + selectedKey + " -> KeyCode: " + keyCode);

            if (keyCode == -1) {
                Log.e("DEBUG", "⚠️ Cod necunoscut pentru tasta: " + selectedKey);
                return false;
            }

            LorieView lorieView = ((MainActivity) context).findViewById(R.id.lorieView);
            if (lorieView == null) {
                Log.e("DEBUG", "❌ Eroare: LorieView nu a fost găsit!");
                return false;
            }

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    // Trimite evenimentul de tastă "apăsat"
                    lorieView.sendKeyEvent(keyCode, keyCode, true);
                    Log.d("DEBUG", "✅ Tasta " + selectedKey + " apăsată.");
                    break;

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    // Trimite evenimentul de tastă "eliberat"
                    lorieView.sendKeyEvent(keyCode, keyCode, false);
                    Log.d("DEBUG", "✅ Tasta " + selectedKey + " eliberată.");
                    break;
            }

            return true; // Returnăm true pentru a consuma evenimentul
        });
    }

    private int getKeyEventCode(String key) {
        switch (key) {
            case "W": return 17; // Scan code pentru W
            case "A": return 30; // Scan code pentru A
            case "S": return 31; // Scan code pentru S
            case "D": return 32; // Scan code pentru D
            case "Space": return 57; // Scan code pentru Space
            case "Enter": return 28; // Scan code pentru Enter
            case "Backspace": return 14; // Scan code pentru Backspace
            case "↑": return 103; // Scan code pentru săgeata sus
            case "↓": return 108; // Scan code pentru săgeata jos
            case "←": return 105; // Scan code pentru săgeata stânga
            case "→": return 106; // Scan code pentru săgeata dreapta
            default: return 0; // Scan code necunoscut
        }
    }
}
