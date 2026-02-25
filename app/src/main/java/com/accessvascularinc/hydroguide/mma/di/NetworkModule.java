package com.accessvascularinc.hydroguide.mma.di;

import com.accessvascularinc.hydroguide.mma.repository.FacilitySyncRepository;

import android.content.Context;

import dagger.hilt.android.qualifiers.ApplicationContext;

import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.NetworkUtils;

import java.io.File;
import java.util.concurrent.TimeUnit;

import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.components.SingletonComponent;
import okhttp3.Cache;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.logging.HttpLoggingInterceptor;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;

@Module
@InstallIn(SingletonComponent.class)
public class NetworkModule {
  private static final String BASE_URL = "https://api-avi-eastus-dev.azurewebsites.net/";

  @Provides
  @Singleton
  public static OkHttpClient provideOkHttpClient(@ApplicationContext Context context) {
    File httpCacheDirectory = new File(context.getCacheDir(), "http-cache");
    Cache cache = new Cache(httpCacheDirectory, 10 * 1024 * 1024);
    HttpLoggingInterceptor logging = new HttpLoggingInterceptor();
    boolean isDebug = false;
    try {
      Class<?> buildConfig = Class.forName("com.accessvascularinc.hydroguide.mma.BuildConfig");
      isDebug = buildConfig.getField("DEBUG").getBoolean(null);
    } catch (Exception ignored) {
    }
    if (isDebug) {
      logging.setLevel(HttpLoggingInterceptor.Level.BODY);
    } else {
      logging.setLevel(HttpLoggingInterceptor.Level.NONE);
    }
    return new OkHttpClient.Builder()
            .cache(cache)
            .connectTimeout(30, TimeUnit.SECONDS)
            .readTimeout(30, TimeUnit.SECONDS)
            .writeTimeout(30, TimeUnit.SECONDS)
            .addInterceptor(logging)
            // Add Authorization header with token from PrefsManager
            .addInterceptor(chain -> {
              Request original = chain.request();
              Request.Builder requestBuilder = original.newBuilder();
              String token = PrefsManager.getTokenFromPrefs(context);
              if (token != null && !token.isEmpty()) {
                requestBuilder.header("Authorization", "Bearer " + token);
              }
              return chain.proceed(requestBuilder.build());
            })
            .addInterceptor(chain -> {
              Request original = chain.request();
              Request.Builder requestBuilder = original.newBuilder()
                      .header("Content-Type", "application/json")
                      .header("Accept", "application/json");
              return chain.proceed(requestBuilder.build());
            })
            .addInterceptor(chain -> {
              Request request = chain.request();
              if (!NetworkUtils.isNetworkAvailable(context)) {
                request = request.newBuilder()
                        .header("Cache-Control", "public, only-if-cached, max-stale=2419200")
                        .build();
              }
              return chain.proceed(request);
            })
            .addNetworkInterceptor(chain -> {
              Response response = chain.proceed(chain.request());
              int maxAge = 60;
              return response.newBuilder()
                      .header("Cache-Control", "public, max-age=" + maxAge)
                      .build();
            })
            .build();
  }

  @Provides
  @Singleton
  public static Retrofit provideRetrofit(OkHttpClient client) {
    return new Retrofit.Builder()
            .baseUrl(BASE_URL)
            .client(client)
            .addConverterFactory(GsonConverterFactory.create())
            .build();
  }

  @Provides
  @Singleton
  public static ApiService provideApiService(Retrofit retrofit) {
    return retrofit.create(ApiService.class);
  }

  @Provides
  @Singleton
  public static FacilitySyncRepository provideFacilitySyncRepository(ApiService apiService) {
    return new FacilitySyncRepository(apiService);
  }
}
