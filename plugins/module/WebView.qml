import QtQuick 2.0
import Ubuntu.Components 1.3
import Ubuntu.Web 0.2

ChromedWebView {
    id: root

    property QtObject signonRequest
    property string userAgent

    onNewViewRequested: {
        var popup = popupComponent.createObject(root, {
            "context": context,
            "request": request,
        })
        popup.closeRequested.connect(function() {
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

    onLoadingStateChanged: {
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
        context.cookieManager.getCookiesResponse.connect(onGotCookies)
        context.cookieManager.getAllCookies()
        visible = false
    }

    function onGotCookies(requestId, cookies) {
        signonRequest.setCookies(cookies)
    }
}
