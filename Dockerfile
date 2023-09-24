FROM ubuntu

ENV QT_DEBUG_PLUGINS=1
ENV QT_QPA_PLATFORM=xcb
ENV QT_QPA_PLATFORM_PLUGIN_PATH=/opt/Qt/${QT_VERSION}/gcc_64/plugins
ENV QT_PLUGIN_PATH=/opt/Qt/${QT_VERSION}/gcc_64/plugins
ENV DISPLAY=:1
ARG path_to_tests_folder="./Tests"

LABEL EMAIL=gorka19800@gmail.com

#install all the necessary libs and apps
RUN apt-get update
RUN apt-get dist-upgrade -y
RUN echo "8"| apt-get install -y qttools5-dev-tools 
RUN apt-get install -y xcb
RUN apt-get install -y git python3 cmake qtbase5-dev g++ libqt5svg5-dev libqt5x11extras5-dev qtscript5-dev libboost-system-dev zlib1g zlib1g-dev

#setup git
RUN git config --global user.email "gorka19800@gmail.com"
RUN git config --global user.name "Test suit"
#clone repo and prepare for building kumir-to-arduino translator
RUN mkdir /home/Sources
WORKDIR /home/Sources/
RUN git clone https://github.com/CaMoCBaJL/kumir2
WORKDIR kumir2/
RUN git pull
RUN git checkout translator_tests
RUN git merge -s ours --no-edit origin/ArduinoFixes
RUN mkdir build
WORKDIR build/
#build translator
RUN cmake -DUSE_QT=5 -DCMAKE_BUILD_TYPE=Release ..
RUN make -j 18
#start tests
WORKDIR ../kumir_tests/
RUN touch test_results.log