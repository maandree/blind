#!/bin/sh
exec convert - -depth 8 -alpha activate pam:- | exec sed '1,/^ENDHDR$/d'
