package com.accessvascularinc.hydroguide.mma.fota;

import java.io.File;
import java.util.List;

public class FirmwareManifest
{
  private int formatVersion;
  private long time;
  private String name;

  private List<FirmwareInfo> files;

  public int getFormatVersion() {
    return formatVersion;
  }

  public void setFormatVersion(final int formatVersion) {
    this.formatVersion = formatVersion;
  }

  public long getTime() {
    return time;
  }

  public void setTime(final long time) {
    this.time = time;
  }

  public String getName() {
    return name;
  }

  public void setName(final String name) {
    this.name = name;
  }

  public List<FirmwareInfo> getFiles() {
    return files;
  }

  public void setFiles(final List<FirmwareInfo> files)
  {
    this.files = files;
  }

  public class FirmwareInfo
  {
    private String type;
    private String board;
    private String soc;
    private int load_address;
    private String image_index;
    private String slot_index_primary;
    private String slot_index_secondary;
    private String version_MCUBOOT;
    private long size;
    private String file;
    private String modTime;

    private File binaryFile;

    public String getType() {
      return type;
    }

    public void setType(final String type) {
      this.type = type;
    }

    public String getBoard() {
      return board;
    }

    public void setBoard(final String board) {
      this.board = board;
    }

    public String getSoc() {
      return soc;
    }

    public void setSoc(final String soc) {
      this.soc = soc;
    }

    public int getLoad_address() {
      return load_address;
    }

    public void setLoad_address(final int load_address) {
      this.load_address = load_address;
    }

    public String getImage_index() {
      return image_index;
    }

    public void setImage_index(final String image_index) {
      this.image_index = image_index;
    }

    public String getSlot_index_primary() {
      return slot_index_primary;
    }

    public void setSlot_index_primary(final String slot_index_primary) {
      this.slot_index_primary = slot_index_primary;
    }

    public String getSlot_index_secondary() {
      return slot_index_secondary;
    }

    public void setSlot_index_secondary(final String slot_index_secondary) {
      this.slot_index_secondary = slot_index_secondary;
    }

    public String getVersion_MCUBOOT() {
      return version_MCUBOOT;
    }

    public void setVersion_MCUBOOT(final String version_MCUBOOT) {
      this.version_MCUBOOT = version_MCUBOOT;
    }

    public long getSize() {
      return size;
    }

    public void setSize(final long size) {
      this.size = size;
    }

    public String getFile() {
      return file;
    }

    public void setFile(final String file) {
      this.file = file;
    }

    public String getModTime() {
      return modTime;
    }

    public void setModTime(final String modTime) {
      this.modTime = modTime;
    }

    public File getBinaryFile() {
      return binaryFile;
    }

    public void setBinaryFile(final File binaryFile) {
      this.binaryFile = binaryFile;
    }
  }
}
