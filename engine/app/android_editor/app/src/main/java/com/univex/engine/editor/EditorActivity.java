package com.univex.engine.editor;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public final class EditorActivity extends Activity {
    private static final int OPEN_PROJECT_REQUEST = 42;
    private static final int OPEN_ASSET_REQUEST = 43;
    private static final long LOADING_DURATION_MS = 900L;
    private static final String PREFS_NAME = "uve_editor_local_state";
    private static final String RECENT_PROJECTS_KEY = "recent_projects";
    private static final int MAX_RECENT_PROJECTS = 5;

    private EditorSurface editorSurface;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private Screen screen = Screen.LOADING;
    private String activeProjectName = "";
    private String activeProjectPath = "";
    private boolean sceneOpen;
    private boolean playing;
    private String selectedNode = "";
    private String inspectorTab = "Inspector";
    private final List<String> sceneNodes = new ArrayList<>();

    private enum Screen {
        LOADING,
        LOBBY,
        EDITOR
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        getWindow().setStatusBarColor(Color.rgb(17, 21, 28));
        getWindow().setNavigationBarColor(Color.rgb(17, 21, 28));
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS,
                WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        editorSurface = new EditorSurface();
        setContentView(editorSurface);
        mainHandler.postDelayed(() -> {
            screen = Screen.LOBBY;
            editorSurface.invalidate();
        }, LOADING_DURATION_MS);
    }

    @Override
    protected void onDestroy() {
        mainHandler.removeCallbacksAndMessages(null);
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        if (screen == Screen.EDITOR) {
            screen = Screen.LOBBY;
            editorSurface.invalidate();
            return;
        }
        super.onBackPressed();
    }

    private void showCreateProjectDialog() {
        final EditText nameInput = new EditText(this);
        nameInput.setSingleLine(true);
        nameInput.setHint("Project name");
        nameInput.setTextColor(Color.WHITE);
        nameInput.setHintTextColor(Color.rgb(154, 168, 186));
        nameInput.setPadding(24, 8, 24, 8);
        new AlertDialog.Builder(this)
                .setTitle("Create project")
                .setMessage("The project will be stored locally on this device.")
                .setView(nameInput)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Create", (dialog, which) -> createProject(nameInput.getText().toString()))
                .show();
    }

    private void createProject(String rawName) {
        final String displayName = rawName.trim();
        final String projectId = sanitizeProjectId(displayName);
        if (displayName.isEmpty() || projectId.isEmpty()) {
            showMessage("Enter a project name first.");
            return;
        }
        final File projectsRoot = new File(getFilesDir(), "projects");
        final File projectRoot = new File(projectsRoot, projectId);
        final File packagePath = new File(projectRoot, projectId + ".uveditor");
        try {
            if (!projectRoot.exists() && !projectRoot.mkdirs()) {
                throw new IOException("Unable to create local project directory.");
            }
            final File contentRoot = new File(projectRoot, "Content");
            final File settingsRoot = new File(projectRoot, "Settings");
            if ((!contentRoot.exists() && !contentRoot.mkdirs()) ||
                    (!settingsRoot.exists() && !settingsRoot.mkdirs())) {
                throw new IOException("Unable to create local project folders.");
            }
            writeTextFile(new File(contentRoot, ".uveassets"), "{\"schemaVersion\":1,\"assets\":[]}\n");
            writeTextFile(new File(settingsRoot, "editor.json"), "{\"schemaVersion\":1,\"editor\":\"android\"}\n");
            writeProjectPackage(packagePath, projectId, displayName);
            openProject(displayName, packagePath.getAbsolutePath());
        } catch (IOException | JSONException exception) {
            showMessage("Could not create project: " + exception.getMessage());
        }
    }

    private void writeTextFile(File path, String content) throws IOException {
        try (OutputStreamWriter writer = new OutputStreamWriter(
                new FileOutputStream(path), StandardCharsets.UTF_8)) {
            writer.write(content);
        }
    }

    private void writeProjectPackage(File packagePath, String projectId, String displayName)
            throws IOException, JSONException {
        final JSONObject version = new JSONObject();
        version.put("major", 0);
        version.put("minor", 1);
        version.put("patch", 0);
        version.put("build", 0);

        final JSONObject packageJson = new JSONObject();
        packageJson.put("format", "uveditor");
        packageJson.put("schemaVersion", 1);
        packageJson.put("revision", 1);
        packageJson.put("projectId", projectId);
        packageJson.put("displayName", displayName);
        packageJson.put("engineVersion", version);
        packageJson.put("contentRoot", "Content");
        packageJson.put("assetDatabasePath", "Content/.uveassets");
        packageJson.put("settingsPath", "Settings/editor.json");

        try (OutputStreamWriter writer = new OutputStreamWriter(
                new FileOutputStream(packagePath), StandardCharsets.UTF_8)) {
            writer.write(packageJson.toString(2));
            writer.write('\n');
        }
    }

    private void openProject(String displayName, String packagePath) {
        activeProjectName = displayName;
        activeProjectPath = packagePath;
        sceneOpen = false;
        playing = false;
        selectedNode = "";
        sceneNodes.clear();
        rememberRecentProject(displayName, packagePath);
        restoreEditorState();
        screen = Screen.EDITOR;
        editorSurface.invalidate();
    }

    private void createScene() {
        sceneOpen = true;
        playing = false;
        sceneNodes.clear();
        sceneNodes.add("Main");
        selectedNode = "Main";
        saveEditorState();
        editorSurface.invalidate();
    }

    private void saveEditorState() {
        if (activeProjectPath.isEmpty()) {
            return;
        }
        try {
            final JSONObject state = new JSONObject();
            state.put("schemaVersion", 1);
            state.put("editor", "android");
            state.put("sceneOpen", sceneOpen);
            state.put("selectedNode", selectedNode);
            final JSONArray nodes = new JSONArray();
            for (String node : sceneNodes) {
                nodes.put(node);
            }
            state.put("sceneNodes", nodes);
            final File settingsFile = new File(new File(activeProjectPath).getParentFile(), "Settings/editor.json");
            writeTextFile(settingsFile, state.toString(2) + "\n");
        } catch (IOException | JSONException exception) {
            showMessage("Could not save editor state: " + exception.getMessage());
        }
    }

    private void restoreEditorState() {
        final File settingsFile = new File(new File(activeProjectPath).getParentFile(), "Settings/editor.json");
        if (!settingsFile.isFile()) {
            return;
        }
        try (FileInputStream input = new FileInputStream(settingsFile)) {
            final ByteArrayOutputStream output = new ByteArrayOutputStream();
            final byte[] buffer = new byte[4096];
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }
            final JSONObject state = new JSONObject(output.toString(StandardCharsets.UTF_8.name()));
            sceneOpen = state.optBoolean("sceneOpen", false);
            selectedNode = state.optString("selectedNode", "");
            final JSONArray nodes = state.optJSONArray("sceneNodes");
            if (nodes != null) {
                for (int index = 0; index < nodes.length(); index++) {
                    sceneNodes.add(nodes.optString(index, ""));
                }
            }
            if (sceneOpen && sceneNodes.isEmpty()) {
                sceneNodes.add("Main");
                selectedNode = "Main";
            }
        } catch (IOException | JSONException exception) {
            sceneOpen = false;
            selectedNode = "";
            sceneNodes.clear();
        }
    }

    private void showAddNodeDialog() {
        if (!sceneOpen) {
            showMessage("Create a scene before adding nodes.");
            return;
        }
        final String[] nodeTypes = {"Node3D", "Camera3D", "MeshInstance3D", "DirectionalLight3D", "Skeleton3D"};
        new AlertDialog.Builder(this)
                .setTitle("Add node")
                .setItems(nodeTypes, (dialog, which) -> {
                    sceneNodes.add(nodeTypes[which]);
                    selectedNode = nodeTypes[which];
                    saveEditorState();
                    editorSurface.invalidate();
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    private void importAssetFromEditor() {
        final Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, OPEN_ASSET_REQUEST);
    }

    private void showOpenProjectPicker() {
        final Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, OPEN_PROJECT_REQUEST);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        final Uri uri = data.getData();
        if (requestCode == OPEN_ASSET_REQUEST) {
            importAsset(uri);
            return;
        }
        if (requestCode != OPEN_PROJECT_REQUEST) {
            return;
        }
        try {
            final String packageText = readDocument(uri);
            final JSONObject packageJson = new JSONObject(packageText);
            if (!"uveditor".equals(packageJson.optString("format"))) {
                showMessage("Selected file is not a .uveditor project.");
                return;
            }
            final String projectId = sanitizeProjectId(packageJson.optString("projectId", "imported_project"));
            final String displayName = packageJson.optString("displayName", projectId);
            final File projectRoot = new File(new File(getFilesDir(), "projects"), projectId);
            if (!projectRoot.exists() && !projectRoot.mkdirs()) {
                throw new IOException("Unable to create local import directory.");
            }
            final File contentRoot = new File(projectRoot, "Content");
            final File settingsRoot = new File(projectRoot, "Settings");
            if ((!contentRoot.exists() && !contentRoot.mkdirs()) ||
                    (!settingsRoot.exists() && !settingsRoot.mkdirs())) {
                throw new IOException("Unable to create local project folders.");
            }
            writeTextFile(new File(contentRoot, ".uveassets"), "{\"schemaVersion\":1,\"assets\":[]}\n");
            writeTextFile(new File(settingsRoot, "editor.json"), "{\"schemaVersion\":1,\"editor\":\"android\"}\n");
            final File localPackage = new File(projectRoot, projectId + ".uveditor");
            try (FileOutputStream output = new FileOutputStream(localPackage)) {
                output.write(packageText.getBytes(StandardCharsets.UTF_8));
            }
            openProject(displayName, localPackage.getAbsolutePath());
        } catch (IOException | JSONException exception) {
            showMessage("Could not open project: " + exception.getMessage());
        }
    }

    private void importAsset(Uri uri) {
        if (activeProjectPath.isEmpty()) {
            showMessage("Open a project before importing assets.");
            return;
        }
        final String rawName = uri.getLastPathSegment() == null ? "asset" : uri.getLastPathSegment();
        final String safeName = rawName.replaceAll("[^a-zA-Z0-9._-]+", "_");
        final File projectRoot = new File(activeProjectPath).getParentFile();
        final File importRoot = new File(projectRoot, "Content/Imported");
        final File destination = new File(importRoot, safeName.isEmpty() ? "asset" : safeName);
        try {
            if (!importRoot.exists() && !importRoot.mkdirs()) {
                throw new IOException("Unable to create the local import folder.");
            }
            try (InputStream input = getContentResolver().openInputStream(uri);
                 FileOutputStream output = new FileOutputStream(destination)) {
                if (input == null) {
                    throw new IOException("The selected asset could not be read.");
                }
                final byte[] buffer = new byte[8192];
                int read;
                long total = 0L;
                while ((read = input.read(buffer)) != -1) {
                    total += read;
                    if (total > 128L * 1024L * 1024L) {
                        throw new IOException("The selected asset is larger than 128 MiB.");
                    }
                    output.write(buffer, 0, read);
                }
            }
            showMessage("Imported locally: " + destination.getName());
        } catch (IOException exception) {
            showMessage("Could not import asset: " + exception.getMessage());
        }
    }

    private String readDocument(Uri uri) throws IOException {
        try (InputStream input = getContentResolver().openInputStream(uri)) {
            if (input == null) {
                throw new IOException("The selected file could not be read.");
            }
            final ByteArrayOutputStream output = new ByteArrayOutputStream();
            final byte[] buffer = new byte[8192];
            int read;
            int total = 0;
            while ((read = input.read(buffer)) != -1) {
                total += read;
                if (total > 1024 * 1024) {
                    throw new IOException("The project manifest is larger than 1 MiB.");
                }
                output.write(buffer, 0, read);
            }
            return output.toString(StandardCharsets.UTF_8.name());
        }
    }

    private void rememberRecentProject(String displayName, String packagePath) {
        final SharedPreferences preferences = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        final List<String> recent = readRecentProjects();
        final String record = displayName.replace('\n', ' ') + "\t" + packagePath;
        recent.removeIf(item -> item.endsWith("\t" + packagePath));
        recent.add(0, record);
        while (recent.size() > MAX_RECENT_PROJECTS) {
            recent.remove(recent.size() - 1);
        }
        preferences.edit().putString(RECENT_PROJECTS_KEY, joinRecords(recent)).apply();
    }

    private List<String> readRecentProjects() {
        final String stored = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                .getString(RECENT_PROJECTS_KEY, "");
        final List<String> records = new ArrayList<>();
        if (stored == null || stored.isEmpty()) {
            return records;
        }
        for (String record : stored.split("\\n")) {
            if (record.contains("\t")) {
                records.add(record);
            }
        }
        return records;
    }

    private String joinRecords(List<String> records) {
        final StringBuilder builder = new StringBuilder();
        for (String record : records) {
            if (builder.length() > 0) {
                builder.append('\n');
            }
            builder.append(record);
        }
        return builder.toString();
    }

    private String sanitizeProjectId(String value) {
        final String lower = value.toLowerCase(Locale.ROOT).replaceAll("[^a-z0-9._-]+", "-");
        return lower.replaceAll("^-+|-+$", "");
    }

    private void showMessage(String message) {
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    private final class EditorSurface extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF createButton = new RectF();
        private final RectF openButton = new RectF();
        private final RectF recentButton = new RectF();
        private final RectF createSceneButton = new RectF();
        private final RectF addNodeButton = new RectF();
        private final RectF importAssetButton = new RectF();
        private final RectF playButton = new RectF();
        private final RectF stopButton = new RectF();
        private final RectF viewportRect = new RectF();
        private final RectF sceneTreeRect = new RectF();
        private final RectF inspectorRect = new RectF();
        private float density;
        private float loadingPhase;

        EditorSurface() {
            super(EditorActivity.this);
            density = getResources().getDisplayMetrics().density;
            setFocusable(true);
        }

        private float dp(float value) {
            return value * density;
        }

        private void fill(Canvas canvas, int color) {
            canvas.drawColor(color);
        }

        private void text(Canvas canvas, String value, float x, float y, float size, int color, boolean medium) {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(color);
            paint.setTextSize(dp(size));
            paint.setTypeface(TypefaceFactory.typeface(medium));
            canvas.drawText(value, dp(x), dp(y), paint);
        }

        private void rounded(Canvas canvas, RectF rect, int color, float radius) {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(color);
            canvas.drawRoundRect(rect, dp(radius), dp(radius), paint);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            if (screen == Screen.LOADING) {
                drawLoading(canvas);
            } else if (screen == Screen.LOBBY) {
                drawLobby(canvas);
            } else {
                drawEditor(canvas);
            }
        }

        private void drawLoading(Canvas canvas) {
            fill(canvas, Color.rgb(17, 21, 28));
            final float centerX = getWidth() / density / 2.0F;
            final float centerY = getHeight() / density / 2.0F - 12.0F;
            text(canvas, "UNIVEX", centerX - 36.0F, centerY, 24.0F, Color.rgb(243, 246, 250), true);
            text(canvas, "EDITOR", centerX - 30.0F, centerY + 25.0F, 11.0F, Color.rgb(154, 168, 186), true);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(dp(2.0F));
            paint.setColor(Color.rgb(94, 141, 255));
            final RectF ring = new RectF(dp(centerX - 22.0F), dp(centerY + 52.0F),
                    dp(centerX + 22.0F), dp(centerY + 96.0F));
            canvas.drawArc(ring, loadingPhase, 250.0F, false, paint);
            loadingPhase = (loadingPhase + 8.0F) % 360.0F;
            text(canvas, "Preparing local workspace", centerX - 65.0F, centerY + 125.0F,
                    12.0F, Color.rgb(154, 168, 186), false);
            postInvalidateDelayed(32L);
        }

        private void drawLobby(Canvas canvas) {
            fill(canvas, Color.rgb(17, 21, 28));
            final float width = getWidth() / density;
            final float height = getHeight() / density;
            text(canvas, "UNIVEX EDITOR", 32.0F, 42.0F, 13.0F, Color.rgb(154, 168, 186), true);
            text(canvas, "Projects", 32.0F, 92.0F, 30.0F, Color.rgb(243, 246, 250), true);
            text(canvas, "Create or open a local project to enter the editor.", 32.0F, 119.0F,
                    13.0F, Color.rgb(154, 168, 186), false);

            final float cardLeft = 32.0F;
            final float cardRight = width - 32.0F;
            rounded(canvas, new RectF(dp(cardLeft), dp(150.0F), dp(cardRight), dp(256.0F)),
                    Color.rgb(26, 32, 42), 12.0F);
            text(canvas, "LOCAL WORKSPACE", 52.0F, 182.0F, 11.0F, Color.rgb(94, 141, 255), true);
            text(canvas, "No account required", 52.0F, 214.0F, 18.0F, Color.rgb(243, 246, 250), true);
            text(canvas, "Projects stay on this device until you export or share them.", 52.0F, 238.0F,
                    12.0F, Color.rgb(154, 168, 186), false);

            final float buttonTop = Math.max(282.0F, height - 116.0F);
            createButton.set(dp(32.0F), dp(buttonTop), dp(width / 2.0F - 10.0F), dp(buttonTop + 56.0F));
            openButton.set(dp(width / 2.0F + 10.0F), dp(buttonTop), dp(width - 32.0F), dp(buttonTop + 56.0F));
            rounded(canvas, createButton, Color.rgb(94, 141, 255), 10.0F);
            rounded(canvas, openButton, Color.rgb(34, 43, 56), 10.0F);
            text(canvas, "+  Create", 52.0F, buttonTop + 35.0F, 15.0F, Color.WHITE, true);
            text(canvas, "Open project", width / 2.0F + 31.0F, buttonTop + 35.0F,
                    14.0F, Color.rgb(243, 246, 250), true);

            final List<String> recent = readRecentProjects();
            if (!recent.isEmpty() && height >= 430.0F) {
                text(canvas, "RECENT", 32.0F, 292.0F, 11.0F, Color.rgb(154, 168, 186), true);
                final String[] fields = recent.get(0).split("\\t", 2);
                if (fields.length == 2) {
                    recentButton.set(dp(32.0F), dp(306.0F), dp(width - 32.0F), dp(360.0F));
                    rounded(canvas, recentButton, Color.rgb(26, 32, 42), 10.0F);
                    text(canvas, fields[0], 52.0F, 338.0F, 15.0F, Color.rgb(243, 246, 250), true);
                    text(canvas, "Tap to reopen local project", width - 215.0F, 338.0F, 11.0F,
                            Color.rgb(154, 168, 186), false);
                }
            }
        }

        private void drawEditor(Canvas canvas) {
            fill(canvas, Color.rgb(17, 21, 28));
            final float width = getWidth() / density;
            final float height = getHeight() / density;
            final float leftPanel = Math.max(142.0F, width * 0.24F);
            final float rightPanel = Math.max(156.0F, width * 0.25F);
            final float centerLeft = leftPanel + 8.0F;
            final float centerRight = width - rightPanel - 8.0F;
            final float contentTop = 66.0F;

            rounded(canvas, new RectF(dp(0.0F), dp(0.0F), dp(width), dp(54.0F)),
                    Color.rgb(26, 32, 42), 0.0F);
            text(canvas, "UNIVEX", 18.0F, 23.0F, 10.0F, Color.rgb(154, 168, 186), true);
            text(canvas, activeProjectName, 18.0F, 43.0F, 15.0F, Color.rgb(243, 246, 250), true);

            playButton.set(dp(width / 2.0F - 58.0F), dp(10.0F), dp(width / 2.0F - 6.0F), dp(44.0F));
            stopButton.set(dp(width / 2.0F + 6.0F), dp(10.0F), dp(width / 2.0F + 58.0F), dp(44.0F));
            rounded(canvas, playButton, playing ? Color.rgb(101, 193, 140) : Color.rgb(34, 43, 56), 8.0F);
            rounded(canvas, stopButton, Color.rgb(34, 43, 56), 8.0F);
            text(canvas, "▶  Play", width / 2.0F - 47.0F, 32.0F, 12.0F,
                    playing ? Color.rgb(17, 21, 28) : Color.rgb(243, 246, 250), true);
            text(canvas, "■  Stop", width / 2.0F + 17.0F, 32.0F, 12.0F, Color.rgb(243, 246, 250), true);
            text(canvas, "⋮", width - 28.0F, 35.0F, 22.0F, Color.rgb(154, 168, 186), false);

            sceneTreeRect.set(dp(12.0F), dp(contentTop), dp(leftPanel), dp(height - 12.0F));
            viewportRect.set(dp(centerLeft), dp(contentTop), dp(centerRight), dp(height - 12.0F));
            inspectorRect.set(dp(centerRight + 8.0F), dp(contentTop), dp(width - 12.0F), dp(height - 12.0F));
            rounded(canvas, sceneTreeRect, Color.rgb(26, 32, 42), 10.0F);
            rounded(canvas, viewportRect, Color.rgb(14, 18, 24), 10.0F);
            rounded(canvas, inspectorRect, Color.rgb(26, 32, 42), 10.0F);

            text(canvas, "SCENE", 26.0F, contentTop + 28.0F, 11.0F, Color.rgb(154, 168, 186), true);
            addNodeButton.set(dp(leftPanel - 42.0F), dp(contentTop + 10.0F), dp(leftPanel - 24.0F), dp(contentTop + 34.0F));
            text(canvas, "+", leftPanel - 40.0F, contentTop + 29.0F, 19.0F, Color.rgb(94, 141, 255), true);
            rounded(canvas, new RectF(dp(24.0F), dp(contentTop + 44.0F), dp(leftPanel - 24.0F), dp(contentTop + 78.0F)),
                    Color.rgb(34, 43, 56), 7.0F);
            text(canvas, "Search nodes", 36.0F, contentTop + 66.0F, 11.0F, Color.rgb(154, 168, 186), false);
            if (!sceneOpen) {
                text(canvas, "No scene open", 26.0F, contentTop + 122.0F, 12.0F, Color.rgb(154, 168, 186), false);
                text(canvas, "Create a scene to begin.", 26.0F, contentTop + 146.0F, 10.0F, Color.rgb(154, 168, 186), false);
            } else {
                float nodeY = contentTop + 112.0F;
                for (String node : sceneNodes) {
                    if (node.equals(selectedNode)) {
                        rounded(canvas, new RectF(dp(20.0F), dp(nodeY - 20.0F), dp(leftPanel - 18.0F), dp(nodeY + 7.0F)),
                                Color.rgb(52, 73, 108), 5.0F);
                    }
                    text(canvas, node.equals("Main") ? "⌄  " + node : "   ↳  " + node, 28.0F, nodeY,
                            12.0F, Color.rgb(243, 246, 250), node.equals(selectedNode));
                    nodeY += 31.0F;
                }
            }

            text(canvas, "VIEWPORT", centerLeft + 14.0F, contentTop + 27.0F, 11.0F,
                    Color.rgb(154, 168, 186), true);
            importAssetButton.set(dp(centerRight - 102.0F), dp(contentTop + 10.0F), dp(centerRight - 18.0F), dp(contentTop + 38.0F));
            rounded(canvas, importAssetButton, Color.rgb(34, 43, 56), 6.0F);
            text(canvas, "Import", centerRight - 83.0F, contentTop + 29.0F, 11.0F, Color.rgb(243, 246, 250), true);
            if (!sceneOpen) {
                createSceneButton.set(dp(centerLeft + 24.0F), dp(contentTop + 126.0F), dp(centerRight - 24.0F), dp(contentTop + 178.0F));
                rounded(canvas, createSceneButton, Color.rgb(94, 141, 255), 9.0F);
                text(canvas, "Create Scene", centerLeft + (centerRight - centerLeft) / 2.0F - 48.0F,
                        contentTop + 158.0F, 14.0F, Color.WHITE, true);
                text(canvas, "Empty workspace", centerLeft + (centerRight - centerLeft) / 2.0F - 46.0F,
                        contentTop + 215.0F, 12.0F, Color.rgb(154, 168, 186), false);
            } else {
                drawViewportGrid(canvas, centerLeft, centerRight, contentTop + 42.0F, height - 12.0F);
                text(canvas, "Perspective", centerLeft + 14.0F, height - 30.0F, 11.0F,
                        Color.rgb(243, 246, 250), true);
                text(canvas, "XYZ", centerRight - 44.0F, height - 30.0F, 11.0F, Color.rgb(94, 141, 255), true);
            }

            text(canvas, "INSPECTOR", centerRight + 16.0F, contentTop + 27.0F, 11.0F,
                    Color.rgb(154, 168, 186), true);
            final String[] tabs = {"Inspector", "Import", "Signals"};
            float tabX = centerRight + 16.0F;
            for (String tab : tabs) {
                if (tab.equals(inspectorTab)) {
                    rounded(canvas, new RectF(dp(tabX - 4.0F), dp(contentTop + 42.0F), dp(tabX + 66.0F), dp(contentTop + 70.0F)),
                            Color.rgb(52, 73, 108), 5.0F);
                }
                text(canvas, tab, tabX, contentTop + 61.0F, 10.0F,
                        tab.equals(inspectorTab) ? Color.rgb(243, 246, 250) : Color.rgb(154, 168, 186), true);
                tabX += tab.equals("Inspector") ? 71.0F : 59.0F;
            }
            if (selectedNode.isEmpty()) {
                text(canvas, "Select a node", centerRight + 16.0F, contentTop + 116.0F, 13.0F,
                        Color.rgb(154, 168, 186), false);
                text(canvas, "Properties will appear here.", centerRight + 16.0F, contentTop + 140.0F, 10.0F,
                        Color.rgb(154, 168, 186), false);
            } else {
                text(canvas, selectedNode, centerRight + 16.0F, contentTop + 112.0F, 16.0F,
                        Color.rgb(243, 246, 250), true);
                text(canvas, "Node3D", centerRight + 16.0F, contentTop + 137.0F, 11.0F,
                        Color.rgb(94, 141, 255), true);
                text(canvas, "Transform", centerRight + 16.0F, contentTop + 184.0F, 11.0F,
                        Color.rgb(154, 168, 186), true);
                text(canvas, "Position     0     0     0", centerRight + 16.0F, contentTop + 211.0F, 11.0F,
                        Color.rgb(243, 246, 250), false);
                text(canvas, "Rotation     0     0     0", centerRight + 16.0F, contentTop + 238.0F, 11.0F,
                        Color.rgb(243, 246, 250), false);
                text(canvas, "Scale        1     1     1", centerRight + 16.0F, contentTop + 265.0F, 11.0F,
                        Color.rgb(243, 246, 250), false);
            }
        }

        private void drawViewportGrid(Canvas canvas, float left, float right, float top, float bottom) {
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(dp(1.0F));
            paint.setColor(Color.rgb(31, 40, 52));
            for (float x = left + 16.0F; x < right; x += 24.0F) {
                canvas.drawLine(dp(x), dp(top), dp(x), dp(bottom), paint);
            }
            for (float y = top + 16.0F; y < bottom; y += 24.0F) {
                canvas.drawLine(dp(left), dp(y), dp(right), dp(y), paint);
            }
            final float centerX = (left + right) / 2.0F;
            final float centerY = (top + bottom) / 2.0F;
            paint.setColor(Color.rgb(208, 87, 87));
            canvas.drawLine(dp(centerX), dp(centerY), dp(centerX + 54.0F), dp(centerY), paint);
            paint.setColor(Color.rgb(101, 193, 140));
            canvas.drawLine(dp(centerX), dp(centerY), dp(centerX), dp(centerY - 54.0F), paint);
            paint.setColor(Color.rgb(94, 141, 255));
            canvas.drawLine(dp(centerX), dp(centerY), dp(centerX - 40.0F), dp(centerY + 40.0F), paint);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() != MotionEvent.ACTION_UP) {
                return true;
            }
            final float x = event.getX();
            final float y = event.getY();
            if (screen == Screen.LOBBY) {
                if (createButton.contains(x, y)) {
                    showCreateProjectDialog();
                } else if (openButton.contains(x, y)) {
                    showOpenProjectPicker();
                } else if (recentButton.contains(x, y)) {
                    final List<String> recent = readRecentProjects();
                    if (!recent.isEmpty()) {
                        final String[] fields = recent.get(0).split("\\t", 2);
                        if (fields.length == 2 && new File(fields[1]).isFile()) {
                            openProject(fields[0], fields[1]);
                        } else {
                            showMessage("The recent project is no longer available.");
                        }
                    }
                }
                return true;
            }
            if (screen != Screen.EDITOR) {
                return true;
            }
            if (!sceneOpen && createSceneButton.contains(x, y)) {
                createScene();
            } else if (addNodeButton.contains(x, y)) {
                showAddNodeDialog();
            } else if (importAssetButton.contains(x, y)) {
                importAssetFromEditor();
            } else if (playButton.contains(x, y)) {
                if (!sceneOpen) {
                    showMessage("Create a scene before pressing Play.");
                } else {
                    playing = true;
                    saveEditorState();
                    invalidate();
                }
            } else if (stopButton.contains(x, y)) {
                playing = false;
                saveEditorState();
                invalidate();
            } else if (sceneOpen && sceneTreeRect.contains(x, y)) {
                final float logicalY = y / density;
                final float top = 66.0F + 112.0F;
                for (int index = 0; index < sceneNodes.size(); index++) {
                    final float nodeY = top + index * 31.0F;
                    if (logicalY >= nodeY - 20.0F && logicalY <= nodeY + 7.0F) {
                        selectedNode = sceneNodes.get(index);
                        saveEditorState();
                        invalidate();
                        break;
                    }
                }
            } else if (inspectorRect.contains(x, y) && y / density >= 108.0F && y / density <= 136.0F) {
                final float logicalX = x / density;
                if (logicalX < inspectorRect.left / density + 76.0F) {
                    inspectorTab = "Inspector";
                } else if (logicalX < inspectorRect.left / density + 135.0F) {
                    inspectorTab = "Import";
                } else {
                    inspectorTab = "Signals";
                }
                invalidate();
            }
            return true;
        }
    }

    private static final class TypefaceFactory {
        private TypefaceFactory() {
        }

        static android.graphics.Typeface typeface(boolean medium) {
            return android.graphics.Typeface.create(
                    medium ? "sans-serif-medium" : "sans-serif", android.graphics.Typeface.NORMAL);
        }
    }
}
