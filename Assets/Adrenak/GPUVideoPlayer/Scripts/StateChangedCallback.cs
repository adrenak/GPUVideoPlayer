using System.Runtime.InteropServices;
using System;

namespace Adrenak.GPUVideoPlayer {
	[StructLayout(LayoutKind.Explicit, Pack = 4)]
	public struct StateChangedMessage {
		[FieldOffset(0)]
		public UInt16 type;

		[FieldOffset(4)]
		public UInt16 state;

		[FieldOffset(4)]
		public Int64 hresult;

		[FieldOffset(4)]
		public Description description;

		[FieldOffset(4)]
		public Int64 position;
	};
}