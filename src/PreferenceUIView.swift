import SwiftUI

struct PreferenceUIView: View {
    @State private var useHeader: Bool = false
    @State private var trackpadRange: String = "0,0,1,1"
    @State private var screenRange: String = "0,0,1,1"

    var isValid: Bool {
        return Settings.Range.stringIsValid(string: trackpadRange,
                                            name: "Trackpad region") &&
               Settings.Range.stringIsValid(string: screenRange,
                                            name: "Screen region")
    }

    var body: some View {
        VStack (alignment: .leading) {
            Toggle("Use settings in header file (settings.h)", isOn: $useHeader)
                .toggleStyle(.checkbox)
            if (!useHeader) {
                Form {
                    TextField("Trackpad region:", text: $trackpadRange)
                    TextField("Screen region:", text: $screenRange)
                }
            }
        }
        Button (action: {
            if isValid && !useHeader {
                settings.trackpadRange = Settings.Range.init(from: trackpadRange)
                settings.screenRange = Settings.Range.init(from: screenRange)
            }
            settings.useHeader = useHeader
        }) {
            Text("Apply").padding()
        }
    }
}
