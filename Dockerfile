FROM ubuntu:latest
LABEL authors="juliano"

ENTRYPOINT ["top", "-b"]