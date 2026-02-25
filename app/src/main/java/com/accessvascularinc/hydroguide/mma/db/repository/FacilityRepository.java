package com.accessvascularinc.hydroguide.mma.db.repository;

import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.dao.FacilityDao;

import jakarta.inject.Singleton;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

@Singleton
public class FacilityRepository {

  private final FacilityDao facilityDao;
  private final ExecutorService executor;

  public FacilityRepository(FacilityDao facilityDao) {
    this.facilityDao = facilityDao;
    this.executor = Executors.newSingleThreadExecutor(); // You can use a larger pool if needed
  }

  public long addFacility(FacilityEntity facility) throws ExecutionException, InterruptedException {
    Callable<Long> task = () -> facilityDao.insertFacility(facility);
    Future<Long> future = executor.submit(task);
    return future.get(); // blocks until result is available
  }

  public void updateFacility(FacilityEntity facility) {
    executor.submit(() -> facilityDao.updateFacility(facility));
  }

  public void deleteFacility(FacilityEntity facility) {
    executor.submit(() -> facilityDao.deleteFacility(facility));
  }

  public FacilityEntity getFacilityById(long id) throws ExecutionException, InterruptedException {
    Callable<FacilityEntity> task = () -> facilityDao.getFacilityById(id);
    Future<FacilityEntity> future = executor.submit(task);
    return future.get();
  }

  public List<FacilityEntity> getFacilitiesByOrganization(String orgId) throws ExecutionException, InterruptedException {
    Callable<List<FacilityEntity>> task = () -> facilityDao.getFacilitiesByOrganization(orgId);
    Future<List<FacilityEntity>> future = executor.submit(task);
    return future.get();
  }

  public List<FacilityEntity> getAllFacilities() throws ExecutionException, InterruptedException {
    Callable<List<FacilityEntity>> task = () -> facilityDao.getAllFacilities();
    Future<List<FacilityEntity>> future = executor.submit(task);
    return future.get();
  }

  public void shutdown() {
    executor.shutdown();
  }
}
