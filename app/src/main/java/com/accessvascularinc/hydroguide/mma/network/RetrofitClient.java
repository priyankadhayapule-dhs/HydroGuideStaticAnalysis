package com.accessvascularinc.hydroguide.mma.network;

import android.content.Context;
import java.io.File;
import java.util.concurrent.TimeUnit;
import okhttp3.Cache;
import okhttp3.CacheControl;
import okhttp3.Interceptor;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.logging.HttpLoggingInterceptor;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

public class RetrofitClient {
    private static final String BASE_URL = "https://api-avi-eastus-dev.azurewebsites.net/";
    private static Retrofit retrofit = null;

    public static Retrofit getClient(Context context) {
        if (retrofit == null) {
            OkHttpClient.Builder clientBuilder = new OkHttpClient.Builder()
                    .connectTimeout(30, TimeUnit.SECONDS)
                    .readTimeout(30, TimeUnit.SECONDS)
                    .writeTimeout(30, TimeUnit.SECONDS);

            // Logging
            HttpLoggingInterceptor logging = new HttpLoggingInterceptor(message -> {
                // Log each line of HTTP request/response
                android.util.Log.d("OkHttp_FULL_BODY", message);
            });
            logging.setLevel(HttpLoggingInterceptor.Level.BODY);
            clientBuilder.addInterceptor(logging);

            // Common headers interceptor - DO NOT override Content-Type for multipart
            // requests
            clientBuilder.addInterceptor(chain -> {
                Request original = chain.request();

                // Only set Content-Type for non-multipart requests
                String existingContentType = original.header("Content-Type");

                Request.Builder requestBuilder = original.newBuilder()
                        .header("Accept", "application/json")
                        .header("Cache-Control", "no-cache")
                        .header("Pragma", "no-cache");

                // Only set Content-Type if it's not already set (multipart has its own)
                if (existingContentType == null) {
                    requestBuilder.header("Content-Type", "application/json");
                }

                return chain.proceed(requestBuilder.build());
            });

            OkHttpClient client = clientBuilder.build();

            retrofit = new Retrofit.Builder()
                    .baseUrl(BASE_URL)
                    .client(client)
                    .addConverterFactory(GsonConverterFactory.create())
                    .build();
        }
        return retrofit;
    }
}
