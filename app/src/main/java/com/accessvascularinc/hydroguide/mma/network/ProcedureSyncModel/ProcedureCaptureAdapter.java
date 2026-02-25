package com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel;

import com.google.gson.*;
import java.lang.reflect.Type;

public class ProcedureCaptureAdapter implements JsonDeserializer<ProcedureSyncResponse.ProcedureCapture> {
    @Override
    public ProcedureSyncResponse.ProcedureCapture deserialize(JsonElement json, Type typeOfT,
            JsonDeserializationContext context) throws JsonParseException {
        JsonObject obj = json.getAsJsonObject();
        ProcedureSyncResponse.ProcedureCapture cap = new ProcedureSyncResponse.ProcedureCapture();
        cap.captureLocalId = obj.has("captureLocalId") ? obj.get("captureLocalId").getAsInt() : 0;
        cap.procedureId = obj.has("procedureId") && !obj.get("procedureId").isJsonNull()
                ? obj.get("procedureId").getAsString()
                : null;
        cap.captureId = obj.has("captureId") ? obj.get("captureId").getAsInt() : 0;
        cap.isIntravascular = obj.has("isIntravascular") && obj.get("isIntravascular").getAsBoolean();
        cap.shownInReport = obj.has("shownInReport") && obj.get("shownInReport").getAsBoolean();
        cap.insertedLengthCm = getDoubleFlexible(obj, "insertedLengthCm");
        cap.exposedLengthCm = getDoubleFlexible(obj, "exposedLengthCm");
        cap.captureData = obj.has("captureData") && !obj.get("captureData").isJsonNull()
                ? obj.get("captureData").getAsString()
                : null;
        cap.markedPoints = obj.has("markedPoints") && !obj.get("markedPoints").isJsonNull()
                ? obj.get("markedPoints").getAsString()
                : null;
        cap.upperBound = getDoubleFlexible(obj, "upperBound");
        cap.lowerBound = getDoubleFlexible(obj, "lowerBound");
        cap.increment = getDoubleFlexible(obj, "increment");
        return cap;
    }

    private double getDoubleFlexible(JsonObject obj, String key) {
        if (!obj.has(key) || obj.get(key).isJsonNull())
            return 0.0;
        JsonElement el = obj.get(key);
        try {
            if (el.isJsonPrimitive()) {
                JsonPrimitive prim = el.getAsJsonPrimitive();
                if (prim.isNumber()) {
                    return prim.getAsDouble();
                } else if (prim.isString()) {
                    return Double.parseDouble(prim.getAsString());
                }
            }
        } catch (Exception e) {
            // ignore, return 0.0
        }
        return 0.0;
    }
}