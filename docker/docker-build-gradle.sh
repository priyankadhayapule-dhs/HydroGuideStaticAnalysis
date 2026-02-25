#!/bin/bash
#
# docker-build-gradle.sh
# Run a gradle build command from within a docker container for Android
# build purposes
#
# Usage:  docker-build-gradle.sh <project-name> <path-to-project-root> <relpath-to-gradlew> <command line arguments to gradlew>
#
# jcochran

# Setup docker image
projectName=$1
projectRootPath=$(realpath $2)
gradlePathOutside=$3
shift
shift
shift

imageName=android-gradle-build
containerName=${projectName}-android-build

# https://stackoverflow.com/questions/59895
thisPath=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Build docker image if necessary
if [[ "$(docker images -l $imageName 2> /dev/null)" == "" ]]; then \
    docker build \
        --debug \
        --progress=plain \
        --build-arg USERNAME=${USER} \
        --build-arg USER_UID=$(id -u ${USER}) \
        --build-arg USER_GID=$(id -g ${USER}) \
        -t $imageName \
        "${thisPath}"

else \
    echo "Docker image ${imageName} already exists.  Skipping Docker image ubild step."
fi

if [[ -z "${BUILD_CONTAINER_NAME}" ]]; then \
    echo BUILD_CONTAINER_NAME is not defined.  Using default value.
else \
    containerName=${BUILD_CONTAINER_NAME}
fi

containerHome=/home/${USER}/sourcecode
projectRootInsidePath=${containerHome}
gradleInsidePath=${containerHome}/${gradlePathOutside}

# Report configuration
echo Using Docker image ${imageName}
echo Using Container Name ${containeName}
echo This file path "${thisPath}"
echo Project name "${projectName}"
echo Project path on outside "${projectRootPath}"
echo Gradle path on outside "${gradlePathOutside}"
echo
echo Project path on inside "${projectRootInsidePath}"
echo Gradle path on inside "${gradleInsidePath}"
echo

## Do the run
set -e

docker run \
    --name ${containerName} \
    --tty \
    --rm \
    -v "${projectRootPath}":"${projectRootInsidePath}" \
    -w "${containerHome}" \
    ${imageName} "${gradlePathOutside}"/gradlew $@



