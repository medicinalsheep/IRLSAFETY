// IRLSAFETY+ iOS — shell UI until I2 AVFoundation preview lands.
// Copyright (c) 2026 IRLSAFETY+ contributors. MIT License.

import SwiftUI

struct ContentView: View {
    var body: some View {
        VStack(spacing: 16) {
            Text("IRLSAFETY+")
                .font(.largeTitle.bold())
                .foregroundStyle(Color(red: 0.24, green: 0.81, blue: 0.56))
            Text("iOS scaffold (I1)")
                .font(.subheadline)
                .foregroundStyle(.secondary)
            Text(
                "Camera preview and on-device detection arrive in phases I2–I5. " +
                    "Until then, stream this phone into OBS (NDI/SRT) and use the Windows filter."
            )
            .font(.body)
            .multilineTextAlignment(.center)
            .padding(.horizontal)
            Text("libirlsafety · local only · no cloud")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black.ignoresSafeArea())
    }
}

#Preview {
    ContentView()
}
