package com.termux.x11.utils;

import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class GLUtility {

    private static final String SIMPLE_SHADER_HEADER = "#version 100\n" +
            "precision highp float;\n";

    private static final String SIMPLE_FRAGMENT_SHADER = SIMPLE_SHADER_HEADER +
            "uniform sampler2D u_Texture;\n" +
            "varying vec2 v_TexCoord;\n" +
            "\n" +
            "void main(void){\n" +
            "    gl_FragColor = texture2D(u_Texture, v_TexCoord);\n" +
            "}";

    private static final String SIMPLE_VERTEX_SHADER = SIMPLE_SHADER_HEADER +
            "attribute vec3 a_Position;\n" +
            "attribute vec2 a_TexCoord;\n" +
            "varying vec2 v_TexCoord;\n" +
            "\n" +
            "void main(void)\n" +
            "{\n" +
            "    gl_Position = vec4(a_Position, 1.0);\n" +
            "    v_TexCoord = a_TexCoord;\n" +
            "}";

    private static final String TAG = "GLUtility";

    public static void bindTexture(int texture) {
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
    }

    public static int createProgram() {
        return createProgram(SIMPLE_VERTEX_SHADER, SIMPLE_FRAGMENT_SHADER);
    }

    public static int createProgram(String vertexShader, String fragmentShader) {
        int program = GLES20.glCreateProgram();
        GLES20.glAttachShader(program, compileShader(GLES20.GL_VERTEX_SHADER, vertexShader));
        GLES20.glAttachShader(program, compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader));
        GLES20.glLinkProgram(program);
        GLES20.glUseProgram(program);
        return program;
    }

    public static int createTexture() {
        int[] texture = new int[1];
        GLES20.glGenTextures(texture.length, texture, 0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture[0]);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);
        return texture[0];
    }

    public static void drawTexture(int program, int texture, float aspect) {
        float[] vertices = new float[] {
                -1, aspect,
                -1, -aspect,
                1, -aspect,
                1, aspect,
        };

        float[] uvs = new float[] {
                0, 1,
                0, 0,
                1, 0,
                1, 1
        };

        short[] indices = new short[] {
                0, 1, 2,
                0, 2, 3
        };

        GLES20.glUseProgram(program);
        attribPointer(program, "a_Position", vertices, 2);
        attribPointer(program, "a_TexCoord", uvs, 2);
        bindTexture(texture);

        GLES20.glDrawElements(
                GLES20.GL_TRIANGLES,
                indices.length,
                GLES20.GL_UNSIGNED_SHORT,
                ByteBuffer.allocateDirect(indices.length * 2)
                        .order(ByteOrder.nativeOrder())
                        .asShortBuffer()
                        .put(indices)
                        .position(0)
        );
    }

    private static void attribPointer(int program, String location, float[] data, int size) {
        int handle = GLES20.glGetAttribLocation(program, location);
        GLES20.glEnableVertexAttribArray(handle);
        GLES20.glVertexAttribPointer(
                handle,
                size,
                GLES20.GL_FLOAT,
                false,
                size * 4,
                ByteBuffer.allocateDirect(data.length * 4)
                        .order(ByteOrder.nativeOrder())
                        .asFloatBuffer()
                        .put(data)
                        .position(0)
        );
    }

    private static int compileShader(int shaderType, String shaderSource) {
        int shader = GLES20.glCreateShader(shaderType);
        if (shader != 0) {
            GLES20.glShaderSource(shader, shaderSource);
            GLES20.glCompileShader(shader);

            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compileStatus, 0);
            if (compileStatus[0] == GLES20.GL_FALSE) {
                Log.e(TAG, "Error compiling shader: " + GLES20.glGetShaderInfoLog(shader));
                GLES20.glDeleteShader(shader);
                shader = 0;
            }
        }
        return shader;
    }
}
