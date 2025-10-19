# Run with:
#     podman build -t scrcpy-builder .

FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    sudo wget unzip gcc git pkg-config meson ninja-build libsdl2-dev \
    libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev \
    libswresample-dev libusb-1.0-0-dev openjdk-21-jdk

RUN groupadd -g 1000 debian \
 && useradd -m -u 1000 -g 1000 -s /bin/bash debian
RUN echo "debian ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

ENV ANDROID_SDK_ROOT=/opt/android/sdk
ENV CMDLINETOOLS_URL=https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip
ENV CMDLINETOOLS_SHA256=7ec965280a073311c339e571cd5de778b9975026cfcbe79f2b1cdcb1e15317ee

RUN wget -q "$CMDLINETOOLS_URL" -O cmdlinetools.zip \
 && echo "$CMDLINETOOLS_SHA256  cmdlinetools.zip" | sha256sum -c

RUN mkdir -p "$ANDROID_SDK_ROOT/tmp" \
 && unzip cmdlinetools.zip -d "$ANDROID_SDK_ROOT/tmp" \
 && mkdir -p "$ANDROID_SDK_ROOT/cmdline-tools" \
 && mv "$ANDROID_SDK_ROOT/tmp/cmdline-tools" "$ANDROID_SDK_ROOT/cmdline-tools/latest" \
 && rmdir "$ANDROID_SDK_ROOT/tmp" \
 && rm cmdlinetools.zip
# platform 36, but build-tools 35 for the current AGP version
RUN yes | $ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager --licenses \
 && $ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager "platform-tools" "platforms;android-36" "build-tools;35.0.0"
RUN chown -R debian:debian $ANDROID_SDK_ROOT

# For scrcpy build scripts
ENV GRADLE_VERSION=8.14.3
ENV GRADLE_HOME=/opt/gradle-$GRADLE_VERSION
ENV GRADLE_URL=https://services.gradle.org/distributions/gradle-$GRADLE_VERSION-bin.zip
ENV GRADLE_SHA256=bd71102213493060956ec229d946beee57158dbd89d0e62b91bca0fa2c5f3531
ENV GRADLE=gradle
ENV PATH=$GRADLE_HOME/bin:$PATH

RUN wget -q "$GRADLE_URL" -O gradle.zip \
 && echo "$GRADLE_SHA256  gradle.zip" | sha256sum -c

RUN unzip gradle.zip -d /opt \
 && rm gradle.zip

USER debian
WORKDIR /home/debian

# Pre-download gradle dependencies for scrcpy
RUN mkdir -p /home/debian/fake-scrcpy/app
COPY fake.gradle /home/debian/fake-scrcpy/build.gradle
COPY fake_app.gradle /home/debian/fake-scrcpy/app/build.gradle
RUN printf "rootProject.name='dummy-android'\ninclude ':app'" > /home/debian/fake-scrcpy/settings.gradle \
 && gradle -p /home/debian/fake-scrcpy dependencies androidDependencies --no-daemon \
 && rm -rf /home/debian/fake-scrcpy
