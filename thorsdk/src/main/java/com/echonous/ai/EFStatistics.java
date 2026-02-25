package com.echonous.ai;

@SuppressWarnings("DeprecatedIsStillUsed")
public class EFStatistics
{
	public float heartRate; // Not able to be edited, so has no original/edited version

	// Typically 0-1, but may be higher than 1 (technically unbounded)
	// When computing statistics, an error is raised if either a4c uncertainty or biplane
	//  uncertainty is higher than 0.2
	public float a4cUncertainty;
	public float a2cUncertainty;
	public float biplaneUncertainty;

	public float originalA4CEDVolume;
	public float originalA4CESVolume;
	public float originalA2CEDVolume;
	public float originalA2CESVolume;
	public float originalBiplaneEDVolume;
	public float originalBiplaneESVolume;
	public float originalA4CStrokeVolume;
	public float originalA4CCardiacOutput;
	public float originalA4CEF;
	public float originalA2CStrokeVolume;
	public float originalA2CCardiacOutput;
	public float originalA2CEF;
	public float originalBiplaneStrokeVolume;
	public float originalBiplaneCardiacOutput;
	public float originalBiplaneEF;

	@Deprecated
	public float originalEF;
	@Deprecated
	public float originalStrokeVolume;
	@Deprecated
	public float originalCardiacOutput;
	@Deprecated
	public float originalEDVolume;
	@Deprecated
	public float originalESVolume;

	public boolean hasEdited;

	public float editedA4CEDVolume;
	public float editedA4CESVolume;
	public float editedA2CEDVolume;
	public float editedA2CESVolume;
	public float editedBiplaneEDVolume;
	public float editedBiplaneESVolume;
	public float editedA4CStrokeVolume;
	public float editedA4CCardiacOutput;
	public float editedA4CEF;
	public float editedA2CStrokeVolume;
	public float editedA2CCardiacOutput;
	public float editedA2CEF;
	public float editedBiplaneStrokeVolume;
	public float editedBiplaneCardiacOutput;
	public float editedBiplaneEF;

	@Deprecated
	public float editedEF;
	@Deprecated
	public float editedStrokeVolume;
	@Deprecated
	public float editedCardiacOutput;
	@Deprecated
	public float editedEDVolume;
	@Deprecated
	public float editedESVolume;

	public boolean isA4CForeshortened;

	public boolean isA2CForeshortened;
}