package com.stsvita.backend;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.Preferences;
import java.util.HashMap;
import java.util.Map;

/**
 * Preferences stored under ux0:data/stsvita/preferences/, one file per name,
 * in the same java.util.Properties-XML format libGDX's desktop backend
 * writes — so a PC "preferences" folder (unlocks, settings, seen cards) can
 * be copied onto the Vita as-is.
 *
 * Serialization uses a small writer/parser because TeaVM lacks
 * java.util.Properties XML.
 */
public class VitaPreferences implements Preferences {

    private final String name;
    private final Map<String, Object> values = new HashMap<>();

    public VitaPreferences(String name) {
        this.name = name;
        load();
    }

    private void load() {
        // Parse DATA/preferences/<name> if present.
    }

    @Override
    public Preferences putBoolean(String key, boolean val) {
        values.put(key, val);
        return this;
    }

    @Override
    public Preferences putInteger(String key, int val) {
        values.put(key, val);
        return this;
    }

    @Override
    public Preferences putLong(String key, long val) {
        values.put(key, val);
        return this;
    }

    @Override
    public Preferences putFloat(String key, float val) {
        values.put(key, val);
        return this;
    }

    @Override
    public Preferences putString(String key, String val) {
        values.put(key, val);
        return this;
    }

    @Override
    public Preferences put(Map<String, ?> vals) {
        values.putAll(vals);
        return this;
    }

    @Override
    public boolean getBoolean(String key) {
        return getBoolean(key, false);
    }

    @Override
    public int getInteger(String key) {
        return getInteger(key, 0);
    }

    @Override
    public long getLong(String key) {
        return getLong(key, 0L);
    }

    @Override
    public float getFloat(String key) {
        return getFloat(key, 0f);
    }

    @Override
    public String getString(String key) {
        return getString(key, "");
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        Object v = values.get(key);
        return v == null ? defValue : Boolean.parseBoolean(v.toString());
    }

    @Override
    public int getInteger(String key, int defValue) {
        Object v = values.get(key);
        return v == null ? defValue : Integer.parseInt(v.toString());
    }

    @Override
    public long getLong(String key, long defValue) {
        Object v = values.get(key);
        return v == null ? defValue : Long.parseLong(v.toString());
    }

    @Override
    public float getFloat(String key, float defValue) {
        Object v = values.get(key);
        return v == null ? defValue : Float.parseFloat(v.toString());
    }

    @Override
    public String getString(String key, String defValue) {
        Object v = values.get(key);
        return v == null ? defValue : v.toString();
    }

    @Override
    public Map<String, ?> get() {
        return new HashMap<>(values);
    }

    @Override
    public boolean contains(String key) {
        return values.containsKey(key);
    }

    @Override
    public void clear() {
        values.clear();
    }

    @Override
    public void remove(String key) {
        values.remove(key);
    }

    @Override
    public void flush() {
        // Write DATA/preferences/<name> atomically (temp + rename).
        Gdx.app.debug("VitaPreferences", "flush(" + name + ") not yet persisted");
    }
}
