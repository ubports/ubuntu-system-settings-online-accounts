import QtQuick 2.9
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.OnlineAccounts.Plugin 1.0
import QtWebEngine 1.7

MainView {
    id: root
backgroundColor : "transparent"
    property var signonRequest: request

    width: units.gu(60)
    height: units.gu(60)


        WebEngineView {
            id: loader
            signonRequest: root.signonRequest
requestActivate();
profile:  WebEngineProfile{
id: webContext
    persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
       property alias dataPath: webContext.persistentStoragePath

            dataPath: dataLocation


    
        httpUserAgent: "Mozilla/5.0 (Linux; Android 8.0.0; Pixel Build/OPR3.170623.007) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.98 Mobile Safari/537.36"
    }
            anchors {
                fill: parent
               // bottomMargin: Math.max(osk.height, cancelButton.height)
            }
        }

     

    
}
