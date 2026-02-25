package com.accessvascularinc.hydroguide.mma.ultrasound;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;

import com.accessvascularinc.hydroguide.mma.R;
//import com.echonous.kosmosapp.app.ui.controller.ImagingFlowType;
import com.accessvascularinc.hydroguide.mma.ultrasound.AppConstants;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Iterator;

public class Ups {

    private static final String TAG="Ups";
    private static Ups singleton;
    private static Builder builder;

    private ArrayList<Organ> uniqueOrganList = new ArrayList<>();
    private ArrayList<Organ> uniqueOrganListAi = new ArrayList<>();
    private ArrayList<GlobalIdObject> globalIdArray = new ArrayList<>();

    private Organ selectedOrgan;
    private Organ selectedOrganAi;

    private  Class stringResourceClass = R.string.class;
    private  Class drawableResourceClass = R.drawable.class;


    public Organ getSelectedOrgan() {
        return selectedOrgan;
    }

    public void setSelectedOrgan(Organ selectedOrgan) {
        this.selectedOrgan = selectedOrgan;
    }


    public Organ getSelectedOrganAi() {
        return selectedOrganAi;
    }

    public void setSelectedOrganAi(Organ selectedOrganAi) {
        this.selectedOrganAi = selectedOrganAi;
    }


    public ArrayList<Organ> getUniqueOrganList() {
        return uniqueOrganList;
    }

    public ArrayList<Organ> getUniqueAiOrganList() {
        return uniqueOrganListAi;
    }

    public static Ups getInstance(){
        if(singleton == null){
            singleton = new Ups();
        }
        return singleton;
    }

    public static Builder getBuilder(){
        if(builder == null){
            builder = new Builder();
        }
        return builder;
    }

    public void buildAi(Builder builder){
        addOrganAi(builder);
    }
    public void build(Builder builder){
        addOrgan(builder);
    }

    public void createGlobalIdArray( Context ctx){
        Resources res = ctx.getResources();
        XmlResourceParser parser = res.getXml(R.xml.organs);

        int eventType = -1;
        while(eventType != XmlResourceParser.END_DOCUMENT)
        {
            if(eventType == XmlResourceParser.START_TAG) {

                try {
                    if (parser.getEventType() == XmlResourceParser.START_TAG) {
                        String name = parser.getName();

                        if (name.equals(AppConstants.ATTR_ORGAN)) {

                            String organNameAttribute = parser.getAttributeValue(null,AppConstants.ATTR_ORGAN_NAME);
                            int organNameResourceId = getLocalResourceId(stringResourceClass, organNameAttribute);
                            String organName = res.getString(organNameResourceId);

                            int globalId = Integer.parseInt(parser.getAttributeValue(null,AppConstants.ATTR_GLOBAL_ID));

                            String iconResourceAttribute = parser.getAttributeValue(null,AppConstants.ATTR_ICON_RESOURCE);
                            Integer drawableResourceId = getLocalResourceId(drawableResourceClass, iconResourceAttribute);

                            GlobalIdObject obj = new GlobalIdObject(globalId, organName, drawableResourceId);
                            globalIdArray.add(obj);
                        }
                    }
                } catch (XmlPullParserException e) {
                    MedDevLog.error(TAG, "Error creating global id array", e);
                }
            }
            try {
                eventType = parser.next();
            } catch (XmlPullParserException e) {
                MedDevLog.error(TAG, "XML Error parsing next event type", e);
            }catch (IOException ioException){
                MedDevLog.error(TAG, "IO Error parsing next event type", ioException);
            }
        }
    }

    private int getLocalResourceId(Class res, String attribute){
        Field field = null;
        try {
            field = res.getField(attribute);
        } catch (NoSuchFieldException e) {
            MedDevLog.error(TAG, "Error getting local resource id", e);
        }
        int resourceId = 0;
        try {
            if (field != null) {
                resourceId = field.getInt(null);
            }
        } catch (IllegalAccessException e) {
            MedDevLog.error(TAG, "Error parsing field", e);
        }

        return resourceId;
    }

    /* Adds an Organ to the unique organs list, if a global ID exists for that organ.
    *  The Global ID array must be initialized to add an organ*/
    public void addOrgan(Builder builder){

        Organ organ = Organ.Builder
                .getInstance()
                .addOrganId(builder.organId)
                .addOrganName(builder.organName)
                .addGlobalId(builder.globalId)
                .addDepthList(builder.depthList)
                .addBmiList(builder.bmiArray)
                .build();

        Iterator<GlobalIdObject> iterator = globalIdArray.iterator();
        boolean found = false;

        GlobalIdObject obj=null;

        while(iterator.hasNext()){
            obj = iterator.next();
            if(obj.globalId.equals(builder.globalId)){
                found = true;
                break;
            }
        }

        if(found){
            organ.setDrawableId(obj.getIconResourceId());
            if (organ.getGlobalId() == com.accessvascularinc.hydroguide.mma.app.AppConstants.ORGAN_GLOBAL_ID_BLADDER) {
                organ.setFlowType(ImagingFlowType.BF.getType());
            } else {
                organ.setFlowType(ImagingFlowType.FF.getType());
            }
            uniqueOrganList.add(organ);
        }
    }

    enum ImagingFlowType{
        FF (0),
        EF (1),
        BF (2);

        private final int type; // Integer value for each enum constant

        // Constructor to set the integer value
        ImagingFlowType(int code) {
            this.type = code;
        }

        // Getter method to retrieve the integer value
        public int getType() {
            return type;
        }
    }

    /* Adds an Organ to the unique organs list, if a global ID exists for that organ.
     *  The Global ID array must be initialized to add an organ*/
    public void addOrganAi(Builder builder){

        Organ organ = Organ.Builder
                .getInstance()
                .addOrganId(builder.organId)
                .addOrganName(builder.organName)
                .addGlobalId(builder.globalId)
                .addDepthList(builder.depthList)
                .addBmiList(builder.bmiArray)
                .build();

        Iterator<GlobalIdObject> iterator = globalIdArray.iterator();
        boolean found = false;

        GlobalIdObject obj=null;

        while(iterator.hasNext()){
            obj = iterator.next();
            if(obj.globalId.equals(builder.globalId)){
                found = true;
                break;
            }
        }

        if(found){
            organ.setDrawableId(obj.getIconResourceId());
            organ.setFlowType(ImagingFlowType.EF.getType());
            uniqueOrganListAi.add(organ);
        }
    }

    public void clearOrgansList(){
        uniqueOrganList.clear();
    }

    public void clearAiOrgansList(){
        uniqueOrganListAi.clear();
    }

    public void clean(){
        uniqueOrganList.clear();
        uniqueOrganListAi.clear();
        globalIdArray.clear();
        selectedOrgan = null;
        selectedOrganAi = null;
        singleton = null;
    }

    public static class Builder{

        private Integer organId;
        private Integer globalId;

        private String organName;
        private String[] bmiArray;
        private Integer[] depthList;

        public Builder(){
            //Empty constructor
        }

        public Builder addOrganId(String organId){
            this.organId = Integer.valueOf(organId);
            return this;
        }

        public Builder addOrganName(String organName){
            this.organName = organName;
            return this;
        }

        public Builder addOrganGlobalId(String organGlobalId){
            this.globalId = Integer.valueOf(organGlobalId);
            return this;
        }

        public Builder addBmi(String[] bmi){
            this.bmiArray = bmi;
            return this;
        }

        public Builder addDepthList(Integer[] depthList){
            this.depthList = depthList;
            return this;
        }

        public void build(){
            singleton.build(this);
        }

        public void buildAi(){
            singleton.buildAi(this);
        }
    }

    private class GlobalIdObject{
        private String  organName;
        private Integer globalId;
        private Integer iconResourceId;

        public GlobalIdObject(Integer globalId, String organName, Integer iconResourceId) {
            this.globalId = globalId;
            this.organName = organName;
            this.iconResourceId = iconResourceId;
        }

        public Integer getGlobalId() {
            return globalId;
        }

        public String getOrganName() {
            return organName;
        }

        public Integer getIconResourceId() {
            return iconResourceId;
        }
    }
}
