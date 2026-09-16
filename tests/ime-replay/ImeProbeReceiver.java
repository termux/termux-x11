package com.termux.x11;
import android.content.*;
import android.os.*;
import android.view.inputmethod.*;
import org.json.*;
import java.io.*;
import java.nio.file.*;
import java.util.concurrent.CountDownLatch;
// Opt-in diagnostic only. Copy into app sources temporarily; never ship in a normal APK.
public class ImeProbeReceiver extends BroadcastReceiver {
  final Context context; final LorieView view; final Handler main=new Handler(Looper.getMainLooper());
  final File status,gate; volatile boolean running;
  ImeProbeReceiver(Context c,LorieView v){context=c;view=v;status=new File(c.getCacheDir(),"ime-probe-status");gate=new File(c.getCacheDir(),"ime-probe-gate");}
  static void register(Context c,LorieView v){c.registerReceiver(new ImeProbeReceiver(c,v),new IntentFilter("com.termux.x11.IME_PROBE"),"android.permission.DUMP",null,Context.RECEIVER_EXPORTED);}
  void write(String value)throws Exception {File tmp=new File(status+".tmp");Files.write(tmp.toPath(),value.getBytes("UTF-8"));tmp.renameTo(status);}
  void await(String value)throws Exception{long deadline=SystemClock.uptimeMillis()+120000;while(SystemClock.uptimeMillis()<deadline){if(gate.exists()&&new String(Files.readAllBytes(gate.toPath()),"UTF-8").trim().equals(value))return;SystemClock.sleep(50);}throw new Exception("timeout "+value);}
  interface Action {void run()throws Exception;}
  void ui(Action action)throws Exception{CountDownLatch latch=new CountDownLatch(1);Throwable[] err={null};main.post(()->{try{action.run();}catch(Throwable e){err[0]=e;}finally{latch.countDown();}});latch.await();if(err[0]!=null)throw new Exception(err[0]);}
  public void onReceive(Context c,Intent intent){if(running)return;running=true;new Thread(()->{try{
    JSONArray cases=new JSONArray(new String(Files.readAllBytes(new File(context.getCacheDir(),"ime-probe-cases.json").toPath()),"UTF-8"));
    InputConnection[] connection={null};ui(()->connection[0]=view.onCreateInputConnection(new EditorInfo()));InputConnection ic=connection[0];
    for(int i=0;i<cases.length();i++){
      JSONObject test=cases.getJSONObject(i);String id=test.getString("id");ui(()->ic.finishComposingText());write("ready|"+id);await(id+":go");
      JSONArray steps=test.getJSONArray("steps");for(int j=0;j<steps.length();j++){
        JSONObject step=steps.getJSONObject(j);String op=step.getString("op"),text=step.optString("text");
        if(op.equals("wait")){SystemClock.sleep(step.getInt("ms"));continue;}
        ui(()->{switch(op){
          case "compose":ic.setComposingText(text,1);break;
          case "commit":ic.commitText(text,1);break;
          case "delete":ic.deleteSurroundingText(step.getInt("before"),step.optInt("after",0));break;
          case "finish":ic.finishComposingText();break;
          case "begin":ic.beginBatchEdit();break;
          case "end":ic.endBatchEdit();break;
          case "raw":view.sendTextEvent(text.getBytes("UTF-8"));break;
          default:throw new IllegalArgumentException(op);
        }});SystemClock.sleep(step.optInt("delay",10));
      }
      CountDownLatch applied=new CountDownLatch(1);
      java.lang.reflect.Field field=LorieView.class.getDeclaredField("mSyncListener");field.setAccessible(true);
      LorieView.SyncListener previous=(LorieView.SyncListener)field.get(view);
      ui(()->{view.setSyncListener(serial->{if(serial==123456)applied.countDown();if(previous!=null)previous.onSyncReply(serial);});view.sendSync(123456);});
      if(!applied.await(60,java.util.concurrent.TimeUnit.SECONDS))throw new Exception("input barrier timeout");
      ui(()->view.setSyncListener(previous));SystemClock.sleep(500);write("done|"+id);await(id+":next");
    }write("complete");
  }catch(Throwable e){try{write("error|"+e);}catch(Exception ignored){}}finally{running=false;}},"IME-probe").start();}
}
