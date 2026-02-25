# Dockerfile for android APK builds.
# Nominally this is generic for any Android build using gradle, but in
# practice it is currently tied to a specific Android SDK.
#
# jcochran

FROM ubuntu:22.04
LABEL maintainer="jcochran@sparxeng.com"

# Match UID and GID with the host system so that files we create will have
# the correct permissions outside the container.
ARG USERNAME=spxdev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Chicago

## Create user account with correct UID and GID, install sudo, and
## give sudo permissions.
RUN    groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

## Setup repos and install required packages
RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
       ca-certificates curl wget sed git subversion dos2unix rename \
       locales nano \
       openjdk-17-jre openjdk-17-jdk \
       android-sdk sdkmanager \
    && rm -rf /var/lib/apt/lists/*

RUN dpkg-reconfigure locales && \
    locale-gen en_US.UTF-8 && \
    update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

ENV LANG=en_US.utf8

## Point Gradle to Android SDK
ENV ANDROID_HOME=/home/${USERNAME}/Android/Sdk


## Drop into user mode
USER $USERNAME

## Install Android SDK updates before dropping root
## TODO:  Support various SDK versions

RUN mkdir -pv /home/${USERNAME}/Android
RUN mkdir -pv /home/${USERNAME}/Android/Sdk
RUN sdkmanager --install "build-tools;35.0.0"
RUN sdkmanager --install "platforms;android-35"
RUN sdkmanager --install "platform-tools;35.0.0"

## "Accept" license agreements
COPY licenses/* /home/${USERNAME}/Android/Sdk/licenses/


## Default shell
CMD ["/bin/bash"] ["--login"]

