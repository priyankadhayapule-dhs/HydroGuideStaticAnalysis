package com.accessvascularinc.hydroguide.mma.db.repository;

import com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity;
import com.accessvascularinc.hydroguide.mma.db.dao.ProcedureCaptureDao;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class ProcedureCaptureRepository {

  private final ProcedureCaptureDao captureDao;
  private final ExecutorService executor;

  public ProcedureCaptureRepository(ProcedureCaptureDao captureDao) {
    this.captureDao = captureDao;
    this.executor = Executors.newSingleThreadExecutor(); // You can use a larger pool if needed
  }

  public long addCapture(ProcedureCaptureEntity capture) throws ExecutionException, InterruptedException {
    Callable<Long> task = () -> captureDao.insertCapture(capture);
    Future<Long> future = executor.submit(task);
    return future.get();
  }

  public void updateCapture(ProcedureCaptureEntity capture) {
    executor.submit(() -> captureDao.updateCapture(capture));
  }

  public void deleteCapture(ProcedureCaptureEntity capture) {
    executor.submit(() -> captureDao.deleteCapture(capture));
  }

  public ProcedureCaptureEntity getCaptureById(long id) throws ExecutionException, InterruptedException {
    Callable<ProcedureCaptureEntity> task = () -> captureDao.getCaptureById(id);
    Future<ProcedureCaptureEntity> future = executor.submit(task);
    return future.get();
  }

  public List<ProcedureCaptureEntity> getCapturesByProcedure(long procedureId) throws ExecutionException, InterruptedException {
    Callable<List<ProcedureCaptureEntity>> task = () -> captureDao.getCapturesByProcedure(procedureId);
    Future<List<ProcedureCaptureEntity>> future = executor.submit(task);
    return future.get();
  }

  public List<ProcedureCaptureEntity> getAllCaptures() throws ExecutionException, InterruptedException {
    Callable<List<ProcedureCaptureEntity>> task = () -> captureDao.getAllCaptures();
    Future<List<ProcedureCaptureEntity>> future = executor.submit(task);
    return future.get();
  }

  public void shutdown() {
    executor.shutdown();
  }
}
