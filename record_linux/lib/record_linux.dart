import 'dart:async';

import 'package:flutter/services.dart';
import 'package:record_platform_interface/record_platform_interface.dart';

/// The Linux implementation of the Record plugin.
/// Communicates with native (C++ or other) code via a [MethodChannel].
class RecordLinux extends RecordPlatform {
  static void registerWith() {
    RecordPlatform.instance = RecordLinux();
  }

  final MethodChannel _channel = const MethodChannel('record_linux');

  /// Internal state of the recorder
  RecordState _state = RecordState.stop;

  /// Broadcasts [RecordState] changes (stop, record, pause, etc.)
  StreamController<RecordState>? _stateStreamCtrl;

  /// Broadcasts PCM bytes when recording in stream mode
  StreamController<Uint8List>? _audioCtrl;

  /// Keep track of the file path passed in [start], so we can return it in [stop].
  String? _recordedFilePath;

  /// --------------------------------------------------------------------------
  ///  create(...)
  ///
  ///  Called before starting a recorder session, if it isn't already created.
  @override
  Future<void> create(String recorderId) async {
    // On Linux, there might be no special "create" logic needed,
    // but we keep this so it parallels other platform implementations.
  }

  /// --------------------------------------------------------------------------
  ///  start(...)
  ///
  ///  Starts a new recording session to a file at [path].
  @override
  Future<void> start(
    String recorderId,
    RecordConfig config, {
    required String path,
  }) async {
    try {
      // Store the path so we can return it later in stop()
      _recordedFilePath = path;

      // Invoke platform logic to start a file-based recording
      await _channel.invokeMethod('startRecordingFile', path);

      _updateState(RecordState.record);
    } on PlatformException catch (e) {
      throw Exception('Failed to start recording: ${e.message}');
    }
  }

  /// --------------------------------------------------------------------------
  ///  startStream(...)
  ///
  ///  Starts a new recording session and returns a [Stream] of raw PCM [Uint8List].
  @override
  Future<Stream<Uint8List>> startStream(
    String recorderId,
    RecordConfig config,
  ) async {
    // Create a broadcast stream controller for PCM data
    _audioCtrl = StreamController<Uint8List>.broadcast();

    // Handle native -> Dart callbacks
    _channel.setMethodCallHandler((MethodCall call) async {
      if (call.method == 'audioData') {
        final bytes = call.arguments as Uint8List?;
        if (bytes != null && !_audioCtrl!.isClosed) {
          _audioCtrl!.add(bytes);
        }
      }
    });

    try {
      // Tell native code to start capturing audio data
      await _channel.invokeMethod('startRecording');
      _updateState(RecordState.record);
    } on PlatformException catch (e) {
      throw Exception('Failed to start stream-based recording: ${e.message}');
    }

    return _audioCtrl!.stream;
  }

  /// --------------------------------------------------------------------------
  ///  stop(...)
  ///
  ///  Stops the current recording session and returns the file path (if any).
  @override
  Future<String?> stop(String recorderId) async {
    // If we're not actively recording, just return whatever our last path was
    if (_state != RecordState.record) {
      return _recordedFilePath;
    }

    try {
      await _channel.invokeMethod('stopRecordingFile');
      _updateState(RecordState.stop);
    } on PlatformException catch (e) {
      throw Exception('Failed to stop recording: ${e.message}');
    }

    return _recordedFilePath;
  }

  /// --------------------------------------------------------------------------
  ///  cancel(...)
  ///
  ///  Stops and discards/deletes the file (if needed).
  @override
  Future<void> cancel(String recorderId) async {
    // If you want to literally delete the file, you can call a separate
    // method on the native side. For now, we just call [stop].
    await stop(recorderId);
  }

  /// --------------------------------------------------------------------------
  ///  pause(...)
  ///
  ///  Pauses recording session.
  @override
  Future<void> pause(String recorderId) async {
    // Not implemented on Linux
    throw UnsupportedError('Pause is not implemented on Linux.');
  }

  /// --------------------------------------------------------------------------
  ///  resume(...)
  ///
  ///  Resumes recording session after [pause].
  @override
  Future<void> resume(String recorderId) async {
    // Not implemented on Linux
    throw UnsupportedError('Resume is not implemented on Linux.');
  }

  /// --------------------------------------------------------------------------
  ///  isRecording(...)
  ///
  ///  Checks if there's a valid recording session (even if paused).
  @override
  Future<bool> isRecording(String recorderId) async {
    return _state == RecordState.record;
  }

  /// --------------------------------------------------------------------------
  ///  isPaused(...)
  ///
  ///  Checks if recording session is paused.
  @override
  Future<bool> isPaused(String recorderId) async {
    return _state == RecordState.pause;
  }

  /// --------------------------------------------------------------------------
  ///  hasPermission(...)
  ///
  ///  Checks and requests for audio record permission if needed.
  @override
  Future<bool> hasPermission(String recorderId) async {
    // On Linux, microphone permission is usually OS-level or always granted
    return true;
  }

  /// --------------------------------------------------------------------------
  ///  listInputDevices(...)
  ///
  ///  Lists capture/input devices available on the platform.
  @override
  Future<List<InputDevice>> listInputDevices(String recorderId) async {
    // Not implemented
    return <InputDevice>[];
  }

  /// --------------------------------------------------------------------------
  ///  getAmplitude(...)
  ///
  ///  Gets current average & max amplitudes (dBFS).
  ///  Linux implementation is stubbed out => returns -160 dBFS for both.
  @override
  Future<Amplitude> getAmplitude(String recorderId) async {
    return Amplitude(current: -160.0, max: -160.0);
  }

  /// --------------------------------------------------------------------------
  ///  isEncoderSupported(...)
  ///
  ///  Checks if the given encoder is supported on the current platform.
  @override
  Future<bool> isEncoderSupported(
    String recorderId,
    AudioEncoder encoder,
  ) async {
    // For the sake of example, let's say only WAV is "supported"
    return encoder == AudioEncoder.wav;
  }

  /// --------------------------------------------------------------------------
  ///  dispose(...)
  ///
  ///  Releases any native resources and closes streams.
  @override
  Future<void> dispose(String recorderId) async {
    // Stop recording if still active
    await stop(recorderId);

    // Close stream controllers
    _stateStreamCtrl?.close();
    _audioCtrl?.close();
    _audioCtrl = null;
  }

  /// --------------------------------------------------------------------------
  ///  onStateChanged(...)
  ///
  ///  Streams [RecordState] changes (pause, record, stop, etc.).
  @override
  Stream<RecordState> onStateChanged(String recorderId) {
    _stateStreamCtrl ??= StreamController<RecordState>.broadcast();
    return _stateStreamCtrl!.stream;
  }

  /// Updates our current [_state] and notifies any listeners on [_stateStreamCtrl].
  void _updateState(RecordState newState) {
    if (_state != newState) {
      _state = newState;
      _stateStreamCtrl?.add(newState);
    }
  }
}
