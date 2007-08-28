From mclasen@redhat.com Mon Oct 17 00:13:50 2005
Subject: more FT_INTERNAL removal
From: Matthias Clasen <mclasen@redhat.com>
To: freetype-devel@nongnu.org
Content-Type: text/plain
Message-Id: <1129522427.2503.4.camel@c-24-218-204-47.hsd1.ma.comcast.net>
Mime-Version: 1.0
X-Mailer: Evolution 2.4.0 (2.4.0-2) 
Date: Mon, 17 Oct 2005 00:13:50 -0400
X-Evolution-Format: text/plain
X-Evolution-Account: 1096119554.21784.0@x1-6-00-50-fc-70-9f-e0
X-Evolution-Transport: smtp://mclasen@localhost:2226/;use_ssl=when-possible
X-Evolution-Fcc: email://local@local/Sent
Content-Transfer-Encoding: 8bit

I have written a patch to make fontconfig no longer use FT_INTERNAL
headers, by copying the ftglue code found in Pango. The patch is at
https://bugs.freedesktop.org/show_bug.cgi?id=4779


Matthias
