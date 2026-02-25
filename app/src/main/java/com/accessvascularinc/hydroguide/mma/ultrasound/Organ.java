package com.accessvascularinc.hydroguide.mma.ultrasound;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * Created by on 2/8/18.
 */

public class Organ {

    private Integer organId;

    private Integer globalId;
    private String organName;
    private Integer drawableId;
    private Integer flowType;

    private ArrayList<Integer> depthList = new ArrayList();
    private ArrayList<String> bmiList= new ArrayList<>();

    private Organ(Builder builder) {
        this.organId = builder.organId;
        this.globalId = builder.globalId;
        this.organName = builder.organName;
        this.drawableId = builder.drawableId;
        this.bmiList = builder.bmiList;
        this.depthList = builder.depthList;
    }

    public ArrayList<Integer> getOrganDepthList(){
        return depthList;
    }

    public ArrayList<String> getOrganBmiList() {
        return bmiList;
    }

    public String getOrganName() {
        return organName;
    }

    public void setOrganName(String organName) {
        this.organName = organName;
    }

    public Integer getDrawableId() {
        return drawableId;
    }

    public void setDrawableId(Integer drawableId) {
        this.drawableId = drawableId;
    }

    public void addGlobalId(Integer globalId){
        this.globalId = globalId;
    }

    public Integer getGlobalId() {
        return globalId;
    }

    public Integer getOrganId() {
        return organId;
    }

    public Integer getFlowType() { return  flowType; }

    public void setFlowType(Integer flowType) {
        this.flowType = flowType;
    }

    public void addBmi(ArrayList<String> bmiList){
        if(bmiList!=null){
            this.bmiList = bmiList;
        }
    }

    @Override
    public boolean equals(Object organ) {

        if(organ instanceof Organ) {
            Integer id = ((Organ)organ).organId;
            return (this.organId.compareTo(id)) == 0;
        }
        return false;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + organId;
        return result;
    }

    public static class Builder{

        private static Builder singleton;
        private Integer organId;
        private ArrayList<String> bmiList;
        private ArrayList<Integer> depthList;
        private String organName;
        private Integer drawableId;
        private Integer globalId;

        public static Builder getInstance(){
            if(singleton == null){
                return new Builder();
            }
            return singleton;
        }

        public Builder addOrganId(Integer organId){
            this.organId = organId;
            return this;
        }

        public Builder addDepthList(Integer[] depthList){
            this.depthList = new ArrayList<>(Arrays.asList(depthList));
            return this;
        }

        public Builder addGlobalId(Integer globalId){
            this.globalId = globalId;
            return this;
        }

        public Builder addBmiList(String[] bmiArray){
            this.bmiList = new ArrayList<>(Arrays.asList(bmiArray));
            return this;
        }

        public Builder addOrganName(String organName){
            this.organName = organName;
            return this;
        }

        public Builder addDrawable(Integer drawableId){
            this.drawableId = drawableId;
            return this;
        }

        public Organ build() {
            return new Organ(this);
        }
    }



}
