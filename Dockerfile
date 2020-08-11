FROM ubuntu:bionic as dev

LABEL MAINTAINER="sklkim@ucdavis.edu"

ENV MAKEFLAGS "-j8"
ENV DEBIAN_FRONTEND noninteractive

COPY ./install_dependencies.sh /tmp/install_dependencies.sh
RUN bash /tmp/install_dependencies.sh

ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/usr/local/lib"
ENV PATH "/mikos/build/run/bin:${PATH}"
ENV PYTHONPATH "/mikos:${PYTHONPATH}"

ENTRYPOINT ["/bin/bash"]
