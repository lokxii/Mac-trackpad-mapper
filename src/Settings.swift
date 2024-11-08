import Cocoa

struct Settings: Codable {
    struct Range: Codable {
        var low: NSPoint
        var up: NSPoint

        init(low: NSPoint, up: NSPoint) {
            self.low = low
            self.up = up
        }

        init?(from string: String) {
            if Settings.Range.stringIsValid(string: string) {
                let compoenents = string.components(separatedBy: ",")
                    .map { CGFloat(Float($0)!) }
                low = NSPoint(x: compoenents[0], y: compoenents[1])
                up = NSPoint(x: compoenents[2], y: compoenents[3])
            } else {
                return nil
            }
        }

        func toString() -> String {
            return "\(low.x),\(low.y),\(up.x),\(up.y)"
        }

        static func stringIsValid(string: String, name: String = "") -> Bool {
            let components = string.components(separatedBy: ",")
                .map({ (s: String) -> Bool in
                    let f = Float(s)
                    return f != nil && (0.0...1.0).contains(f!)
                })
            let isValid = components.count == 4 && components.reduce(true) { $0 && $1 }
            if !isValid && name != "" {
                alert(msg: name + " range not valid")
            }
            return isValid
        }
    }

    var useHeader: Bool = false
    var trackpadRange: Range? = Range(from: "0.05,0.1,0.95,0.9")
    var screenRange: Range? = nil
    var emitMouseEvent: Bool = false
    var requireCommandKey: Bool = true

    init(trackpad: Range? = nil, screen: Range? = nil) {
        trackpadRange = trackpad
        screenRange = screen
    }

    func toArgs() -> [String] {
        var args: [String] = []

        if let trackpadRange = trackpadRange {
            args.append("-i")
            args.append(trackpadRange.toString())
        }
        if let screenRange = screenRange {
            args.append("-o")
            args.append(screenRange.toString())
        }
        if emitMouseEvent {
            args.append("-e")
        }
        if requireCommandKey {
            args.append("-c")
        }

        return args
    }
}
