import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Web 0.2

WebView {
    property QtObject signonRequest

    Component.onCompleted: {
        signonRequest.authenticated.connect(onAuthenticated)
        url = signonRequest.startUrl
    }

    onLoadingChanged: {
        console.log("Loading changed")
        if (loading) {
            signonRequest.onLoadStarted()
        } else if (lastLoadSucceeded) {
            signonRequest.onLoadFinished(true)
        } else {
            signonRequest.onLoadFinished(false)
        }
    }
    onUrlChanged: signonRequest.currentUrl = url

    context: WebContext {
        dataPath: signonRequest ? signonRequest.rootDir : ""
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
