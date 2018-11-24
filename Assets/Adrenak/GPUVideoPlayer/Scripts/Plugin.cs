using System;
using System.Runtime.InteropServices;

namespace Adrenak.GPUVideoPlayer {
	public static class Plugin {
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

		public delegate void StateChangedCallback(StateChangedMessage args);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "CreateMediaPlayback")]
		public static extern long CreateMediaPlayback(StateChangedCallback callback);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "ReleaseMediaPlayback")]
		public static extern void ReleaseMediaPlayback();

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "CreatePlaybackTexture")]
		public static extern long CreatePlaybackTexture(UInt32 width, UInt32 height, out System.IntPtr playbackTexture);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "LoadContent")]
		public static extern long LoadContent([MarshalAs(UnmanagedType.BStr)] string sourceURL);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "Play")]
		public static extern long Play();

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "Pause")]
		public static extern long Pause();

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "Stop")]
		public static extern long Stop();

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetPosition")]
		public static extern long GetPosition(out Int64 position);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetDuration")]
		public static extern long GetDuration(out Int64 duration);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetPlaybackRate")]
		public static extern long GetPlaybackRate(out Double rate);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "SetPlaybackRate")]
		public static extern long SetPlaybackRate(Double rate);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "SetPosition")]
		public static extern long SetPosition(Int64 position);

		// Unity plugin
		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "SetTimeFromUnity")]
		public static extern void SetTimeFromUnity(float t);

		[DllImport("MediaPlayback", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetRenderEventFunc")]
		public static extern IntPtr GetRenderEventFunc();
	}

}