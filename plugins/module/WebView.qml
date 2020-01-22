import QtQuick 2.9
import Ubuntu.Components 1.3
import Morph.Web 0.1

ChromedWebView {
    id: root

    property QtObject signonRequest
    property string userAgent

    onNewViewRequested: {
        var popup = popupComponent.createObject(root, {
            "context": context,
            "url": request.requestedUrl,
        })
        popup.windowCloseRequested.connect(function() {
            console.log("Close requested!")
            popup.destroy()
        })
    }

    Component {
        id: popupComponent
        ChromedWebView {
            anchors.fill: parent
        }
    }

    onSignonRequestChanged: if (signonRequest) {
        signonRequest.authenticated.connect(onAuthenticated)
        url = signonRequest.startUrl
    }

    onLoadingChanged: {
        console.log("Loading changed")
        if (loading && !lastLoadFailed) {
            signonRequest.onLoadStarted()
        } else if (lastLoadSucceeded) {
            signonRequest.onLoadFinished(true)
        } else if (lastLoadFailed) {
            signonRequest.onLoadFinished(false)
        }
    }
    onUrlChanged: signonRequest.currentUrl = url

    context: WebContext {
        dataPath: signonRequest ? signonRequest.rootDir : ""
        userAgent: root.userAgent ? root.userAgent : defaultUserAgent
    }

    function onAuthenticated() {
        /* Get the cookies and set them on the request */
        console.log("Authenticated; getting cookies")
        /* We could use the QWebEngineCookieStore class to get the cookies.
         * This are then copied over to the webapps, in order to reduce login
         * prompts and support multi account. However, the multi-account
         * feature never landed, so for the moment this is not a priority.
         *
         * Let's just fake it and set empty cookies.

        context.cookieManager.getCookiesResponse.connect(onGotCookies)
        context.cookieManager.getAllCookies()
         */
        visible = false
        onGotCookies(0, [])
    }

    function onGotCookies(requestId, cookies) {
        signonRequest.setCookies(cookies)
    }
}
