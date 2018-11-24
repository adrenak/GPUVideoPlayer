using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Adrenak.GPUVideoPlayer {
	[Serializable]
	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	public struct Description {
		public UInt32 width;
		public UInt32 height;
		public Int64 duration;
		public byte isSeekable;

		public override string ToString() {
			StringBuilder sb = new StringBuilder();
			sb.AppendLine("width: " + width);
			sb.AppendLine("height: " + height);
			sb.AppendLine("duration: " + duration);
			sb.AppendLine("canSeek: " + isSeekable);

			return sb.ToString();
		}
	};
}