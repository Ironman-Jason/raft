#!/bin/sh

ps | grep "example" | awk '{print $1}' | xargs -I{} kill -9 {}
