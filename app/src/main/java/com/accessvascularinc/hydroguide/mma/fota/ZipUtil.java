package com.accessvascularinc.hydroguide.mma.fota;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class ZipUtil
{
  private static final int BUFFER_SIZE = 8192;

  public static List<File> locateFirmwareFile(File zipFile, File destDirectory) throws IOException
  {
    if (!zipFile.exists())
    {
      throw new IOException("ZIP file does not exist: " + zipFile.getAbsolutePath());
    }

    try (FileInputStream fis = new FileInputStream(zipFile))
    {
      return extractZip(fis, destDirectory);
    }
  }

  public static List<File> extractZip(InputStream inputStream, File destDirectory) throws IOException
  {
    List<File> extractedFiles = new ArrayList<>();

    if (!destDirectory.exists())
    {
      if (!destDirectory.mkdirs())
      {
        throw new IOException("Failed to create destination directory: " + destDirectory.getAbsolutePath());
      }
    }

    String destPath = destDirectory.getCanonicalPath();

    try (ZipInputStream zis = new ZipInputStream(new BufferedInputStream(inputStream)))
    {
      ZipEntry entry;

      while ((entry = zis.getNextEntry()) != null)
      {
        String entryName = entry.getName();
        File destFile = new File(destDirectory, entryName);

        String destFilePath = destFile.getCanonicalPath();
        if (!destFilePath.startsWith(destPath + File.separator))
        {
          throw new IOException("ZIP entry is outside of the target directory: " + entryName);
        }

        if (entry.isDirectory())
        {
          if (!destFile.exists() && !destFile.mkdirs())
          {
            throw new IOException("Failed to create directory: " + destFile.getAbsolutePath());
          }
        }
        else
        {
          File parentDir = destFile.getParentFile();
          if (parentDir != null && !parentDir.exists())
          {
            if (!parentDir.mkdirs())
            {
              throw new IOException("Failed to create parent directory: " + parentDir.getAbsolutePath());
            }
          }
          extractFile(zis, destFile);
          extractedFiles.add(destFile);
        }

        zis.closeEntry();
      }
    }

    return extractedFiles;
  }

  private static void extractFile(ZipInputStream zis, File destFile) throws IOException
  {
    try (BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(destFile), BUFFER_SIZE))
    {
      byte[] buffer = new byte[BUFFER_SIZE];
      int bytesRead;

      while ((bytesRead = zis.read(buffer)) != -1)
      {
        bos.write(buffer, 0, bytesRead);
      }
    }
  }

  public static List<String> listEntries(File zipFile) throws IOException
  {
    List<String> entries = new ArrayList<>();
    if (!zipFile.exists())
    {
      throw new IOException("ZIP file does not exist: " + zipFile.getAbsolutePath());
    }

    try (FileInputStream fis = new FileInputStream(zipFile);
      ZipInputStream zis = new ZipInputStream(new BufferedInputStream(fis)))
    {
      ZipEntry entry;
      while ((entry = zis.getNextEntry()) != null)
      {
        entries.add(entry.getName());
        zis.closeEntry();
      }
    }

    return entries;
  }

  public static boolean containsEntry(File zipFile, String entryName) throws IOException
  {
    List<String> entries = listEntries(zipFile);
    return entries.contains(entryName);
  }
}
