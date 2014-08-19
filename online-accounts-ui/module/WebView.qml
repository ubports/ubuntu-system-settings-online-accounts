import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.Extras.Browser 0.2
import com.canonical.Oxide 1.0

UbuntuWebView {
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

    context: UbuntuWebContext {
        dataPath: signonRequest.rootDir
    }

    function onAuthenticated() {
        /* Get the cookies and set them on the request */
        console.log("Authenticated; getting cookies")
        context.cookieManager.gotCookies.connect(onGotCookies)
        context.cookieManager.getAllCookies()
    }

    function onGotCookies(count, cookies, requestStatus) {
        console.log("Got cookies")
        signonRequest.setCookies(cookies)
    }
}
